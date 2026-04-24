#pragma once

#include "ConnectionManager.hpp"
#include "network_config.hpp"

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
    TcpServer(asio::io_context& io, network_config config, IMessageHandler& handler);

    ~TcpServer();

    TcpServer(TcpServer const&) = delete;
    TcpServer& operator=(TcpServer const&) = delete;
    TcpServer(TcpServer&&) = delete;
    TcpServer& operator=(TcpServer&&) = delete;

    asio::awaitable<void> start();
    void stop() noexcept;

private:
    asio::ip::tcp::acceptor m_acceptor;
    network_config m_config;
    IMessageHandler& m_handler;
    ConnectionManager m_connectionManager;
    bool m_stopped{false};
};
} // namespace s2d::network
