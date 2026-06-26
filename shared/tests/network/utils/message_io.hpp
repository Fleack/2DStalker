#pragma once

#include "network_test_constants.hpp"
#include "shared/network/MessageChannel.hpp"

#include <chrono>
#include <cstdint>

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/use_awaitable.hpp>
#include <catch2/catch_test_macros.hpp>

namespace s2d::test::network
{

inline void close_socket(asio::ip::tcp::socket& socket) noexcept
{
    asio::error_code ignored;
    socket.close(ignored);
}

template <typename Message>
asio::awaitable<Message> read_message(
    asio::ip::tcp::socket& socket,
    std::uint32_t max_bytes = max_message_bytes)
{
    s2d::network::MessageChannel channel{max_bytes};
    co_return co_await channel.readMessage<Message>(socket);
}

template <typename Message>
asio::awaitable<void> write_message(
    asio::ip::tcp::socket& socket,
    Message const& message,
    std::uint32_t max_bytes = max_message_bytes)
{
    s2d::network::MessageChannel channel{max_bytes};
    co_await channel.writeMessage(socket, message);
}

inline asio::awaitable<std::size_t> available_bytes_after_wait(
    asio::ip::tcp::socket& socket,
    std::chrono::milliseconds wait_for = std::chrono::milliseconds{10})
{
    asio::steady_timer timer{socket.get_executor()};
    timer.expires_after(wait_for);
    co_await timer.async_wait(asio::use_awaitable);

    asio::error_code ec;
    auto const available_bytes = socket.available(ec);
    REQUIRE_FALSE(ec);

    co_return available_bytes;
}

} // namespace s2d::test::network
