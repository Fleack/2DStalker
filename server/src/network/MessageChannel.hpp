#pragma once

#include <cstdint>
#include <vector>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>

namespace s2d::protocol
{
class ServerMessage;
class ClientMessage;
} // namespace s2d::protocol

namespace s2d::network
{
class MessageChannel
{
public:
    explicit MessageChannel(std::uint32_t max_message_size = 1024 * 1024) noexcept
        : m_max_message_size(max_message_size)
    {
    }

    asio::awaitable<protocol::ClientMessage> readClientMessage(asio::ip::tcp::socket& socket);
    asio::awaitable<void> writeServerMessage(asio::ip::tcp::socket& socket, protocol::ServerMessage const& message);

private:
    std::uint32_t m_max_message_size;
    std::vector<char> m_read_buffer;
};
} // namespace s2d::network
