#pragma once

#include "shared/protocol/message.pb.h"

#include <cstdint>
#include <vector>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

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
        : m_read_buffer(max_message_size), m_max_message_size(max_message_size)
    {
    }

    template <class MessageType>
    asio::awaitable<MessageType> readMessage(asio::ip::tcp::socket& socket)
    {
        std::uint32_t net_size = 0;
        co_await asio::async_read(socket, asio::buffer(&net_size, sizeof(net_size)), asio::use_awaitable);

        std::uint32_t const size = toLittleEndian(net_size);
        if (size > m_max_message_size)
            throw std::runtime_error("Received message frame is bigger than max_message_size");

        m_read_buffer.resize(size);
        if (size > 0)
        {
            co_await asio::async_read(socket, asio::buffer(m_read_buffer.data(), size), asio::use_awaitable);
        }

        MessageType message;
        void const* buffer = size == 0 ? nullptr : m_read_buffer.data();
        if (!message.ParseFromArray(buffer, static_cast<int>(size)))
            throw std::runtime_error("Failed to parse received message to protobuf");

        co_return message;
    }

    template <class MessageType>
    asio::awaitable<void> writeMessage(asio::ip::tcp::socket& socket, MessageType const& message) const
    {

        std::string data;
        if (!message.SerializeToString(&data))
        {
            throw std::runtime_error("Failed to serialize message");
        }

        if (data.size() > m_max_message_size)
        {
            throw std::runtime_error("Serialized server message is bigger than max_message_size");
        }

        std::uint32_t const len = toLittleEndian(static_cast<std::uint32_t>(data.size()));

        co_await asio::async_write(socket, asio::buffer(&len, sizeof(len)), asio::use_awaitable);
        co_await asio::async_write(socket, asio::buffer(data), asio::use_awaitable);
    }

private:
    static std::uint32_t toLittleEndian(std::uint32_t value) noexcept
    {
        if constexpr (std::endian::native == std::endian::little)
        {
            return std::byteswap(value);
        }

        return value;
    }

private:
    std::vector<char> m_read_buffer;
    std::uint32_t m_max_message_size;
};
} // namespace s2d::network
