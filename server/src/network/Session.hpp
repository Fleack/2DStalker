#pragma once

#include <cstdint>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

namespace s2d::protocol
{
class ClientMessage;
class ServerMessage;
} // namespace s2d::protocol

namespace s2d::network
{
class Session
{
public:
    explicit Session(asio::ip::tcp::socket&& socket) noexcept;

    asio::awaitable<void> start();
    void stop() noexcept;

private:
    asio::awaitable<s2d::protocol::ClientMessage> read_request();
    asio::awaitable<void> write_response(const s2d::protocol::ServerMessage& message);

    static s2d::protocol::ServerMessage dispatch(const s2d::protocol::ClientMessage& request);

    asio::ip::tcp::socket m_socket;
};
} // namespace s2d::network
