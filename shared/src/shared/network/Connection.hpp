#pragma once

#include "shared/network/ConnectionWriter.hpp"
#include "shared/network/MessageChannel.hpp"
#include "shared/network/NetworkSide.hpp"
#include "shared/network/connection_id.hpp"
#include "shared/protocol/message.pb.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

namespace s2d::network
{

template <ANetworkSide NetworkSide>
class Connection : public std::enable_shared_from_this<Connection<NetworkSide>>
{
public:
    using incoming_message_t = NetworkSide::incoming_message_t;
    using outcoming_message_t = NetworkSide::outcoming_message_t;
    using handler_t = NetworkSide::handler_t;

    explicit Connection(
        connection_id id,
        asio::ip::tcp::socket&& socket,
        handler_t& handler,
        std::uint32_t max_message_bytes,
        std::function<void(connection_id)> onClosed) noexcept;

    void start();
    void stop();

    void send(outcoming_message_t message);

    [[nodiscard]] connection_id getId() const noexcept
    {
        return m_id;
    }

private:
    asio::awaitable<void> run();
    asio::awaitable<void> readLoop();

    void closeAndReport() noexcept;
    [[nodiscard]] std::string remoteEndpointString() const noexcept;

private:
    asio::ip::tcp::socket m_socket;
    ConnectionWriter<NetworkSide> m_writer;
    MessageChannel m_messageChannel;
    std::function<void(connection_id)> m_onClosed;
    connection_id m_id;
    handler_t& m_handler;
    std::atomic_bool m_stopped{false};
    std::atomic_bool m_closeReported{false};
};

// ============================================================

template <ANetworkSide NetworkSide>
Connection<NetworkSide>::Connection(
    connection_id id,
    asio::ip::tcp::socket&& socket,
    typename NetworkSide::handler_t& handler,
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

template <ANetworkSide NetworkSide>
void Connection<NetworkSide>::start()
{
    auto const remoteEndpoint = remoteEndpointString();
    LOG(info, "Connection[id={}] started with {}", m_id.id, remoteEndpoint);

    auto self = this->shared_from_this();
    asio::co_spawn(
        m_socket.get_executor(),
        [self]() -> asio::awaitable<void> { co_await self->run(); },
        asio::detached);
}

template <ANetworkSide NetworkSide>
void Connection<NetworkSide>::stop()
{
    if (m_stopped.exchange(true))
    {
        LOG(warn, "Connection[id={}] already stopped", m_id.id);
        return;
    }

    auto const remoteEndpoint = remoteEndpointString();

    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    m_socket.close();

    LOG(info, "Connection[id={}] stopped with {}", m_id.id, remoteEndpoint);
}

template <ANetworkSide NetworkSide>
void Connection<NetworkSide>::send(outcoming_message_t message)
{
    if (m_stopped.load() || !m_socket.is_open())
    {
        LOG(warn, "Connection[id={}] is stopped or socket is closed", m_id.id);
        return;
    }

    m_writer.send(std::move(message), this->shared_from_this());
}

// --- private ---

template <ANetworkSide NetworkSide>
asio::awaitable<void> Connection<NetworkSide>::run()
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

template <ANetworkSide NetworkSide>
asio::awaitable<void> Connection<NetworkSide>::readLoop()
{
    for (;;)
    {
        auto message = co_await m_messageChannel.readMessage<incoming_message_t>(m_socket);
        LOG(debug, "Received from connection[{}] message with request_id {}", m_id.id, message.request_id()); // TODO: improve logging
        auto response = co_await m_handler.onMessage(m_id, message);
        response.set_request_id(message.request_id());
        send(std::move(response));
    }
}

template <ANetworkSide NetworkSide>
void Connection<NetworkSide>::closeAndReport() noexcept
{
    if (m_closeReported.exchange(true))
    {
        return;
    }

    stop();

    m_handler.onDisconnect(m_id);

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

template <ANetworkSide NetworkSide>
std::string Connection<NetworkSide>::remoteEndpointString() const noexcept
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
