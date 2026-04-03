#include <asio.hpp>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using asio::ip::tcp;

// --- session ---
asio::awaitable<void> session(tcp::socket socket)
{
    try
    {
        spdlog::info("Client connected");

        for (;;)
        {
            // --- read length ---
            uint32_t net_len;
            co_await asio::async_read(socket, asio::buffer(&net_len, sizeof(net_len)), asio::use_awaitable);

            uint32_t len = ntohl(net_len);

            // --- read body ---
            std::string data(len, '\0');
            co_await asio::async_read(socket, asio::buffer(data), asio::use_awaitable);

            auto j = nlohmann::json::parse(data);
            spdlog::info("Received: {}", j.dump());

            // --- response ---
            nlohmann::json resp = {{"status", "ok"}, {"echo", j}};

            std::string out = resp.dump();
            uint32_t out_len = htonl(static_cast<uint32_t>(out.size()));

            co_await asio::async_write(socket, asio::buffer(&out_len, sizeof(out_len)), asio::use_awaitable);
            co_await asio::async_write(socket, asio::buffer(out), asio::use_awaitable);
        }
    }
    catch (std::exception& e)
    {
        spdlog::warn("Client disconnected: {}", e.what());
    }
}

// --- listener ---
asio::awaitable<void> listener(uint16_t port)
{
    auto executor = co_await asio::this_coro::executor;

    tcp::acceptor acceptor(executor, {tcp::v4(), port});

    spdlog::info("Server started on port {}", port);

    for (;;)
    {
        tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);

        co_spawn(executor, session(std::move(socket)), asio::detached);
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
        spdlog::error("Fatal: {}", e.what());
    }
}
