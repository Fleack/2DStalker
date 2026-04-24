#include "TcpServer.hpp"

#include "logger/logger.hpp"
#include "network_config.hpp"

#include <utility>

#include <asio/error.hpp>
#include <asio/redirect_error.hpp>
#include <asio/use_awaitable.hpp>

namespace s2d::network
{

TcpServer::TcpServer(asio::io_context& io, network_config config, IMessageHandler& handler)
    : m_acceptor(io)
    , m_config(config)
    , m_handler(handler)
{
    using asio::ip::tcp;

    tcp::endpoint const endpoint{tcp::v4(), m_config.port};

    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(tcp::acceptor::reuse_address(true));
    m_acceptor.bind(endpoint);
    m_acceptor.listen();
}

TcpServer::~TcpServer()
{
    stop();
}

asio::awaitable<void> TcpServer::start()
{
    using asio::ip::tcp;

    LOG(info, "Server started on port {}", m_config.port);
    for (;;)
    {
        asio::error_code ec;
        tcp::socket socket{m_acceptor.get_executor()};
        co_await m_acceptor.async_accept(socket, asio::redirect_error(asio::use_awaitable, ec));

        if (ec)
        {
            if (m_stopped && (ec == asio::error::operation_aborted || ec == asio::error::bad_descriptor))
            {
                break;
            }

            LOG(warn, "Failed to accept connection on port {}: {}", m_config.port, ec.message());
            if (m_stopped)
            {
                break;
            }

            continue;
        }

        auto connection = m_connectionManager.create(std::move(socket), m_handler, m_config.max_message_bytes);
        if (!connection)
        {
            break;
        }

        connection->start();
    }

    LOG(info, "Server accept loop stopped on port {}", m_config.port);
}

void TcpServer::stop() noexcept
{
    if (std::exchange(m_stopped, true))
    {
        return;
    }

    LOG(info, "Stopping server on port {}", m_config.port);

    m_acceptor.cancel();
    m_acceptor.close();
    m_connectionManager.stopAll();
}

} // namespace s2d::network
