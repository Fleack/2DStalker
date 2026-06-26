#pragma once

#include <chrono>
#include <future>
#include <utility>

#include <asio/co_spawn.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address_v4.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/use_future.hpp>

namespace s2d::test::network
{

struct io_fixture
{
    asio::io_context io;

    template <typename Awaitable>
    auto spawn(Awaitable&& awaitable)
    {
        return asio::co_spawn(io, std::forward<Awaitable>(awaitable), asio::use_future);
    }

    void run()
    {
        io.run();
    }

    template <typename Rep, typename Period>
    void run_for(std::chrono::duration<Rep, Period> const& duration)
    {
        io.run_for(duration);
    }

    template <typename T, typename Rep, typename Period>
    bool run_until_ready(
        std::future<T> const& future,
        std::chrono::duration<Rep, Period> const& timeout,
        std::chrono::milliseconds step = std::chrono::milliseconds{1})
    {
        auto const deadline = std::chrono::steady_clock::now() + timeout;
        while (future.wait_for(std::chrono::seconds{0}) != std::future_status::ready &&
               std::chrono::steady_clock::now() < deadline)
        {
            io.run_for(step);
        }

        return future.wait_for(std::chrono::seconds{0}) == std::future_status::ready;
    }

    void restart()
    {
        io.restart();
    }
};

struct socket_pair_fixture : io_fixture
{
    struct socket_pair
    {
        asio::ip::tcp::socket client;
        asio::ip::tcp::socket server;
    };

    socket_pair_fixture()
        : sockets{
              asio::ip::tcp::socket{io},
              asio::ip::tcp::socket{io}}
    {
        asio::ip::tcp::acceptor acceptor{io, {asio::ip::tcp::v4(), 0}};
        asio::ip::tcp::endpoint const endpoint{
            asio::ip::address_v4::loopback(),
            acceptor.local_endpoint().port(),
        };

        auto accepted = acceptor.async_accept(asio::use_future);
        auto connected = sockets.client.async_connect(endpoint, asio::use_future);

        io.run();
        connected.get();
        sockets.server = accepted.get();
        io.restart();
    }

    socket_pair sockets;
};

} // namespace s2d::test::network
