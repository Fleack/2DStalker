#pragma once

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

    asio::awaitable<void> send(protocol::ServerMessage const& message);

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
    connection_id m_id;
    asio::ip::tcp::socket m_socket;
    MessageChannel m_messageChannel;
    IMessageHandler& m_handler;
    std::function<void(connection_id)> m_onClosed;
    std::atomic_bool m_stopped{false};
};
} // namespace s2d::network
