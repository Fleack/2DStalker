#include "logger/logger.hpp"
#include "network/Session.hpp"

#include <asio.hpp>
#include <cstdint>

#include <spdlog/spdlog.h>

using asio::ip::tcp;

// --- listener ---
asio::awaitable<void> listener(uint16_t port)
{
    auto executor = co_await asio::this_coro::executor;

    tcp::acceptor acceptor(executor, {tcp::v4(), port});

    LOG(info, "Server started on port {}", port);

    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
        LOG(info, "Client connected: {}", socket.remote_endpoint().address().to_string());

        co_spawn(executor, [socket = std::move(socket)]() mutable -> asio::awaitable<void> {
                     s2d::network::Session session(std::move(socket));
                     co_await session.start(); }, asio::detached);
    }
}

int main()
{
    try
    {
        asio::io_context io;

        co_spawn(io, listener(1234), asio::detached);

        io.run();
    }
    catch (std::exception& e)
    {
        LOG(err, "Fatal: {}", e.what());
    }
}
