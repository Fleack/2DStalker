#include "Client.hpp"

#include "shared/logger/logger.hpp"
#include "shared/protocol/message.pb.h"

#include <asio/connect.hpp>
#include <asio/redirect_error.hpp>
#include <asio/use_awaitable.hpp>

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

asio::awaitable<s2d::protocol::ServerMessage> Client::send(s2d::protocol::ClientMessage const& message)
{
    if (!m_connected)
    {
        LOG(err, "Not connected to server");
        throw std::runtime_error("Not connected to server");
    }
    co_await m_messageChannel.writeMessage(m_socket, message);
    co_return co_await m_messageChannel.readMessage<s2d::protocol::ServerMessage>(m_socket);
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
