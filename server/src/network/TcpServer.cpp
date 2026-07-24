#include "server/src/network/TcpServer.hpp"

#include "server/src/network/tcp_server_config.hpp"
#include "shared/logger/logger.hpp"

#include <utility>

#include <boost/asio/error.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace s2d::network
{

TcpServer::TcpServer(boost::asio::io_context& io, tcp_server_config config, ServerMessageHandler& handler)
    : m_acceptor(io)
    , m_config(config)
    , m_handler(handler)
{
    using boost::asio::ip::tcp;

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

boost::asio::awaitable<void> TcpServer::start()
{
    using boost::asio::ip::tcp;

    LOG(info, "Server started on port {}", m_config.port);
    for (;;)
    {
        system::error_code ec;
        tcp::socket socket{m_acceptor.get_executor()};
        co_await m_acceptor.async_accept(socket, boost::asio::redirect_error(boost::asio::use_awaitable, ec));

        if (ec)
        {
            if (m_stopped && (ec == boost::asio::error::operation_aborted || ec == boost::asio::error::bad_descriptor))
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
