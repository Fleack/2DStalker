#include "Connection.hpp"

#include "IMessageHandler.hpp"
#include "shared/logger/logger.hpp"
#include "shared/protocol/message.pb.h"

#include <exception>
#include <utility>

#include <asio/co_spawn.hpp>
#include <asio/detached.hpp>
#include <asio/error_code.hpp>
#include <fmt/format.h>

namespace s2d::network
{

Connection::Connection(
    connection_id id,
    asio::ip::tcp::socket&& socket,
    IMessageHandler& handler,
    std::uint32_t max_message_bytes,
    std::function<void(connection_id)> onClosed) noexcept
    : m_socket{std::move(socket)}
    , m_writer{m_socket, id, max_message_bytes, [this] { closeAndReport(); }}
    , m_messageChannel{max_message_bytes}
    , m_onClosed{std::move(onClosed)}
    , m_id{id}
    , m_handler{handler}
{
}

void Connection::start()
{
    auto const remoteEndpoint = remoteEndpointString();
    LOG(info, "Connection[id={}] started with client {}", m_id.id, remoteEndpoint);

    auto self = shared_from_this();
    asio::co_spawn(
        m_socket.get_executor(),
        [self]() -> asio::awaitable<void> { co_await self->run(); },
        asio::detached);
}

void Connection::stop() noexcept
{
    if (m_stopped.exchange(true))
    {
        LOG(warn, "Connection[id={}] already stopped", m_id.id);
        return;
    }

    auto const remoteEndpoint = remoteEndpointString();

    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    m_socket.close();

    LOG(info, "Connection[id={}] stopped with client {}", m_id.id, remoteEndpoint);
}

void Connection::send(protocol::ServerMessage message)
{
    if (m_stopped.load() || !m_socket.is_open())
    {
        LOG(warn, "Connection[id={}] is stopped or socket is closed", m_id.id);
        return;
    }

    m_writer.send(std::move(message), shared_from_this());
}

// --- private ---

asio::awaitable<void> Connection::run()
{
    try
    {
        co_await readLoop();
    }
    catch (std::exception const& e)
    {
        if (!m_stopped.load())
        {
            LOG(warn, "Connection[id={}] error during read loop: {}", m_id.id, e.what());
        }
    }

    closeAndReport();
    co_return;
}

asio::awaitable<void> Connection::readLoop()
{
    for (;;)
    {
        auto message = co_await m_messageChannel.readMessage<protocol::ClientMessage>(m_socket);
        LOG(debug, "Received from client[{}] message with request_id {}", m_id.id, message.request_id()); // TODO: improve logging
        auto response = co_await m_handler.onMessage(m_id, message);
        response.set_request_id(message.request_id());
        send(std::move(response));
    }
}

void Connection::closeAndReport() noexcept
{
    if (m_closeReported.exchange(true))
    {
        return;
    }

    stop();

    try
    {
        m_handler.onDisconnect(m_id);
    }
    catch (std::exception const& e)
    {
        LOG(err, "Connection[id={}] disconnect handler failed: {}", m_id.id, e.what());
    }

    if (m_onClosed)
    {
        try
        {
            m_onClosed(m_id);
        }
        catch (std::exception const& e)
        {
            LOG(err, "Connection[id={}] cleanup callback failed: {}", m_id.id, e.what());
        }
    }
}

std::string Connection::remoteEndpointString() const noexcept
{
    try
    {
        auto const endpoint = m_socket.remote_endpoint();
        return fmt::format("{}:{}", endpoint.address().to_string(), endpoint.port());
    }
    catch (...)
    {
        return "<unknown>";
    }
}

} // namespace s2d::network
