#pragma once

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>

class Client
{
public:
    explicit Client(asio::io_context& ctx);

    asio::awaitable<void> connect();
    asio::awaitable<void> send(nlohmann::json const& j);

private:
    asio::ip::tcp::socket socket_;
};
