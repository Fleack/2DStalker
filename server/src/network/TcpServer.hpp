#pragma once

#include "server/src/network/ServerConnectionManager.hpp"
#include "server/src/network/tcp_server_config.hpp"

#include <asio/awaitable.hpp>

namespace asio
{
class io_context;
}

namespace s2d::network
{

class TcpServer
{
public:
    TcpServer(asio::io_context& io, tcp_server_config config, ServerMessageHandler& handler);

    ~TcpServer();

    TcpServer(TcpServer const&) = delete;
    TcpServer& operator=(TcpServer const&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    asio::awaitable<void> start();
    void stop() noexcept;

private:
    asio::ip::tcp::acceptor m_acceptor;
    tcp_server_config m_config;
    ServerMessageHandler& m_handler;
    ServerConnectionManager m_connectionManager;
    bool m_stopped{false};
};
} // namespace s2d::network
