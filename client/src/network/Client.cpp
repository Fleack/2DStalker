#include "Client.hpp"

#include "logger/logger.hpp"

#include <asio/connect.hpp>
#include <asio/read.hpp>
#include <asio/redirect_error.hpp>
#include <asio/use_awaitable.hpp>
#include <asio/write.hpp>

Client::Client(asio::io_context& ctx)
    : m_socket(ctx) {}

Client::~Client()
{
    if (m_connected)
    {
        disconnect();
    }
}

asio::awaitable<void> Client::connect(asio::ip::address ip, uint16_t port)
{
    m_endpoint = asio::ip::tcp::endpoint{ip, port};
    asio::error_code ec;
    co_await m_socket.async_connect(m_endpoint, asio::redirect_error(asio::use_awaitable, ec));
    handle_connect(ec);
}

void Client::disconnect()
{
    if (!m_connected)
    {
        LOG(warn, "Already disconnected from server");
        return;
    }

    m_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
    m_socket.close();
    m_connected = false;
    LOG(info, "Disconnected from server[{}:{}]", m_endpoint.address().to_string(), m_endpoint.port());
    m_endpoint = {};
}

asio::awaitable<std::string> Client::send(nlohmann::json const& j)
{
    if (!m_connected)
    {
        LOG(err, "Not connected to server");
        co_return "Failed to connect to server";
    }
    std::string data = j.dump();
    uint32_t len = htonl(static_cast<uint32_t>(data.size()));

    co_await asio::async_write(m_socket, asio::buffer(&len, sizeof(len)), asio::use_awaitable);
    co_await asio::async_write(m_socket, asio::buffer(data), asio::use_awaitable);

    LOG(debug, "Sent to server[{}:{}]: {}", m_endpoint.address().to_string(), m_endpoint.port(), data);

    uint32_t net_len;
    co_await asio::async_read(m_socket, asio::buffer(&net_len, sizeof(net_len)), asio::use_awaitable);

    u_long resp_len{ntohl(net_len)};
    std::string resp(resp_len, '\0');
    co_await asio::async_read(m_socket, asio::buffer(resp), asio::use_awaitable);
    co_return resp;
}

void Client::handle_connect(asio::error_code ec)
{
    if (ec)
    {
        LOG(err, "Failed to connect to server[{}:{}]: {}", m_endpoint.address().to_string(), m_endpoint.port(), ec.message());
        return;
    }

    m_connected = true;
    LOG(info, "Connected to server[{}:{}]", m_endpoint.address().to_string(), m_endpoint.port());
}
