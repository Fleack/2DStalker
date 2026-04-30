#pragma once

#include "shared/network/MessageChannel.hpp"

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

namespace s2d::protocol
{
class ClientMessage;
}

class Client
{
public:
    explicit Client(asio::io_context& ctx);

    ~Client();

    asio::awaitable<void> connect(asio::ip::address host, uint16_t port);
    void disconnect();

    asio::awaitable<s2d::protocol::ServerMessage> send(s2d::protocol::ClientMessage const& message);

private:
    void handle_connect(asio::error_code ec);

private:
    asio::ip::tcp::socket m_socket;
    s2d::network::MessageChannel m_messageChannel;
    asio::ip::tcp::endpoint m_endpoint{};
    bool m_connected{false};
};
