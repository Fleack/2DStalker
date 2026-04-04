#include "Client.hpp"

#include "logger/logger.hpp"

#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

Client::Client(asio::io_context& ctx)
    : socket_(ctx) {}

asio::awaitable<void> Client::connect()
{
    auto executor = co_await asio::this_coro::executor;

    asio::ip::tcp::resolver resolver(executor);
    auto endpoints = co_await resolver.async_resolve("127.0.0.1", "1234", asio::use_awaitable);

    co_await asio::async_connect(socket_, endpoints, asio::use_awaitable);

    LOG(info, "Connected to server");
}

asio::awaitable<void> Client::send(nlohmann::json const& j)
{
    std::string data = j.dump();
    uint32_t len = htonl(static_cast<uint32_t>(data.size()));

    co_await asio::async_write(socket_, asio::buffer(&len, sizeof(len)), asio::use_awaitable);
    co_await asio::async_write(socket_, asio::buffer(data), asio::use_awaitable);

    uint32_t net_len;
    co_await asio::async_read(socket_, asio::buffer(&net_len, sizeof(net_len)), asio::use_awaitable);

    uint32_t resp_len = ntohl(net_len);

    std::string resp(resp_len, '\0');
    co_await asio::async_read(socket_, asio::buffer(resp), asio::use_awaitable);
}
