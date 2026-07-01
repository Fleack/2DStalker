#pragma once

#include "shared/network/MessageFrameCodec.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

namespace s2d::network
{
class MessageChannel
{
public:
    explicit MessageChannel(std::uint32_t max_message_size = 1024 * 1024) noexcept;

    template <class MessageType>
    asio::awaitable<MessageType> readMessage(asio::ip::tcp::socket& socket);

    template <class MessageType>
    asio::awaitable<void> writeMessage(asio::ip::tcp::socket& socket, MessageType message) const;

private:
    MessageFrameCodec m_frameCodec;
    std::vector<char> m_readBuffer;
};

inline MessageChannel::MessageChannel(std::uint32_t max_message_size) noexcept
    : m_frameCodec{max_message_size}, m_readBuffer(max_message_size)
{
}

template <class MessageType>
asio::awaitable<MessageType> MessageChannel::readMessage(asio::ip::tcp::socket& socket)
{
    MessageFrameCodec::LengthPrefix prefix{};
    co_await asio::async_read(socket, asio::buffer(prefix), asio::use_awaitable);

    std::uint32_t const size = m_frameCodec.decodeLengthPrefix(prefix);

    m_readBuffer.resize(size);
    co_await asio::async_read(socket, asio::buffer(m_readBuffer.data(), size), asio::use_awaitable);

    MessageType message;
    if (!message.ParseFromArray(m_readBuffer.data(), static_cast<int>(size)))
        throw std::runtime_error("Failed to parse received message to protobuf");

    co_return message;
}

template <class MessageType>
asio::awaitable<void> MessageChannel::writeMessage(asio::ip::tcp::socket& socket, MessageType message) const
{
    std::string data;
    if (!message.SerializeToString(&data))
    {
        throw std::runtime_error("Failed to serialize message");
    }

    if (data.size() > static_cast<std::size_t>(m_frameCodec.maxPayloadSize()))
    {
        throw std::runtime_error("Serialized server message is bigger than max_message_size");
    }

    auto const prefix = m_frameCodec.encodeLengthPrefix(static_cast<std::uint32_t>(data.size()));

    co_await asio::async_write(socket, asio::buffer(prefix), asio::use_awaitable);
    co_await asio::async_write(socket, asio::buffer(data), asio::use_awaitable);
}
} // namespace s2d::network
