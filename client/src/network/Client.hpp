#pragma once

#include <asio/awaitable.hpp>
#include <asio/ip/tcp.hpp>
#include <nlohmann/json.hpp>

class Client
{
public:
    explicit Client(asio::io_context& ctx);

    ~Client();

    asio::awaitable<void> connect(asio::ip::address host, uint16_t port);
    void disconnect();

    asio::awaitable<std::string> send(nlohmann::json const& j);

private:
    void handle_connect(asio::error_code ec);

private:
    bool m_connected{false};
    asio::ip::tcp::endpoint m_endpoint{};
    asio::ip::tcp::socket m_socket;

};
