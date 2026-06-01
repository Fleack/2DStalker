#include "ConnectionWriter.hpp"

#include "shared/logger/logger.hpp"

#include <exception>
#include <utility>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/post.hpp>

namespace s2d::network
{

ConnectionWriter::ConnectionWriter(
    asio::ip::tcp::socket& socket,
    connection_id id,
    std::uint32_t max_message_bytes,
    std::function<void()> onError)
    : m_strand{socket.get_executor()}
    , m_messageChannel{max_message_bytes}
    , m_onError{std::move(onError)}
    , m_socket{socket}
    , m_id{id}
{
}

void ConnectionWriter::send(protocol::ServerMessage message, std::shared_ptr<Connection> connection)
{
    asio::post(
        m_strand,
        [this, message = std::move(message), connection = std::move(connection)]() mutable {
            enqueue(std::move(message), std::move(connection));
        });
}

void ConnectionWriter::enqueue(protocol::ServerMessage message, std::shared_ptr<Connection> connection)
{
    if (!m_socket.is_open())
    {
        LOG(warn, "Connection[id={}] is stopped or socket is closed", m_id.id);
        return;
    }

    m_queue.push_back(std::move(message));

    if (m_writeInProgress)
    {
        return;
    }

    m_writeInProgress = true;
    asio::co_spawn(
        m_strand,
        [this, connection = std::move(connection)]() mutable -> asio::awaitable<void> {
            co_await drain(std::move(connection));
        },
        asio::detached);
}

asio::awaitable<void> ConnectionWriter::drain(std::shared_ptr<Connection> connection)
{
    while (!m_queue.empty())
    {
        auto message = std::move(m_queue.front());
        m_queue.pop_front();

        try
        {
            LOG(debug, "Sending to client[id={}] message[id={}] with status {}", m_id.id, message.request_id(), std::to_underlying(message.status())); // TODO: improve logging
            co_await m_messageChannel.writeMessage(m_socket, message);
        }
        catch (std::exception const& e)
        {
            LOG(warn, "Connection[id={}] error during write: {}", m_id.id, e.what());
            m_queue.clear();
            m_writeInProgress = false;
            m_onError();
            co_return;
        }
    }

    m_writeInProgress = false;
    (void)connection;
    co_return;
}

} // namespace s2d::network
