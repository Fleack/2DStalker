#include "MessageChannel.hpp"

#include <message.pb.h>

#include <bit>
#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>
#include <stdexcept>

namespace s2d::network
{
namespace
{
std::uint32_t toNetworkOrder(std::uint32_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
    {
        return std::byteswap(value);
    }

    return value;
}

std::uint32_t fromNetworkOrder(std::uint32_t value) noexcept
{
    if constexpr (std::endian::native == std::endian::little)
    {
        return std::byteswap(value);
    }

    return value;
}
} // namespace

asio::awaitable<void> MessageChannel::writeServerMessage(asio::ip::tcp::socket& socket, protocol::ServerMessage const& message)
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

    std::uint32_t const len = toNetworkOrder(static_cast<std::uint32_t>(data.size()));

    co_await asio::async_write(socket, asio::buffer(&len, sizeof(len)), asio::use_awaitable);
    co_await asio::async_write(socket, asio::buffer(data), asio::use_awaitable);
}

asio::awaitable<protocol::ClientMessage> MessageChannel::readClientMessage(asio::ip::tcp::socket& socket)
{
    std::uint32_t net_size = 0;
    co_await asio::async_read(socket, asio::buffer(&net_size, sizeof(net_size)), asio::use_awaitable);

    std::uint32_t const size = fromNetworkOrder(net_size);
    if (size > m_max_message_size)
        throw std::runtime_error("Received message frame is bigger than max_message_size");

    m_read_buffer.resize(size);
    if (size > 0)
    {
        co_await asio::async_read(socket, asio::buffer(m_read_buffer.data(), size), asio::use_awaitable);
    }

    protocol::ClientMessage message;
    void const* buffer = size == 0 ? nullptr : m_read_buffer.data();
    if (!message.ParseFromArray(buffer, static_cast<int>(size)))
        throw std::runtime_error("Failed to parse received message to protobuf");

    co_return message;
}

} // namespace s2d::network
