#pragma once

#include "ConnectionWriter.hpp"
#include "connection_id.hpp"
#include "shared/network/MessageChannel.hpp"
#include "shared/protocol/message.pb.h"

#include <atomic>
#include <functional>
#include <memory>
#include <string>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

namespace s2d::protocol
{
class ClientMessage;
class ServerMessage;
} // namespace s2d::protocol

namespace s2d::network
{
class IMessageHandler;

class Connection : public std::enable_shared_from_this<Connection>
{
public:
    explicit Connection(
        connection_id id,
        asio::ip::tcp::socket&& socket,
        IMessageHandler& handler,
        std::uint32_t max_message_bytes,
        std::function<void(connection_id)> onClosed) noexcept;

    void start();
    void stop() noexcept;

    void send(protocol::ServerMessage message);

    connection_id getId() const noexcept
    {
        return m_id;
    }

private:
    asio::awaitable<void> run();
    asio::awaitable<void> readLoop();

    void closeAndReport() noexcept;
    std::string remoteEndpointString() const noexcept;

private:
    asio::ip::tcp::socket m_socket;
    ConnectionWriter m_writer;
    MessageChannel m_messageChannel;
    std::function<void(connection_id)> m_onClosed;
    connection_id m_id;
    IMessageHandler& m_handler;
    std::atomic_bool m_stopped{false};
    std::atomic_bool m_closeReported{false};
};
} // namespace s2d::network
