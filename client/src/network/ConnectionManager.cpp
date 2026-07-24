#include "ConnectionManager.hpp"

#include <stdexcept>
#include <system_error>
#include <utility>

#include <boost/asio/as_tuple.hpp>
#include <boost/asio/cancel_after.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace network
{

ConnectionManager::ConnectionManager(
    asio::strand<asio::any_io_executor> strand,
    Config const& config,
    std::shared_ptr<ClientRequestManager> manager)
    : m_strand{std::move(strand)}
    , m_connectTimeout{config.connectTimeout}
    , m_manager{std::move(manager)}
    , m_disconnectCompleted{m_strand, 1}
{
    if (!m_manager)
    {
        throw std::invalid_argument{"ConnectionManager requires request manager"};
    }
}

asio::awaitable<void> ConnectionManager::connect(asio::ip::tcp::endpoint endpoint)
{
    [[maybe_unused]] auto selfGuard = shared_from_this();
    co_await asio::dispatch(m_strand, asio::use_awaitable);

    if (state() != ConnectionState::Disconnected)
    {
        throw std::logic_error{"Connect is not allowed in current connection state"};
    }

    setState(ConnectionState::Connecting);
    auto& socket = m_connectingSocket.emplace(m_strand);

    auto [error] = co_await socket.async_connect(
        endpoint,
        asio::cancel_after(
            m_connectTimeout,
            asio::as_tuple(asio::use_awaitable)));

    if (state() == ConnectionState::Disconnecting)
    {
        closeConnectingSocket();
        m_connectingSocket.reset();
        setState(ConnectionState::Disconnected);
        throw std::system_error{
            asio::error::make_error_code(asio::error::operation_aborted),
            "Connection attempt was cancelled"};
    }

    if (error)
    {
        closeConnectingSocket();
        m_connectingSocket.reset();
        setState(ConnectionState::Disconnected);

        if (error == asio::error::operation_aborted)
        {
            throw std::runtime_error{"Connection timed out"};
        }

        throw std::system_error{static_cast<std::error_code>(error), "Failed to connect to server"};
    }

    auto connectedSocket = std::move(*m_connectingSocket);
    m_connectingSocket.reset();

    try
    {
        auto weak = weak_from_this();
        m_manager->start(
            std::move(connectedSocket),
            [weak]() {
                if (auto manager = weak.lock())
                {
                    manager->connectionClosed();
                }
            });
        setState(ConnectionState::Connected);
    }
    catch (...)
    {
        setState(ConnectionState::Disconnected);
        throw;
    }
}

asio::awaitable<void> ConnectionManager::disconnect()
{
    [[maybe_unused]] auto selfGuard = shared_from_this();
    co_await asio::dispatch(m_strand, asio::use_awaitable);

    auto const currentState = state();
    if (currentState != ConnectionState::Connecting && currentState != ConnectionState::Connected)
    {
        throw std::logic_error{"Disconnect is not allowed in current connection state"};
    }

    setState(ConnectionState::Disconnecting);

    if (currentState == ConnectionState::Connecting)
    {
        closeConnectingSocket();
    }
    else
    {
        m_manager->stop("client disconnected");
    }

    co_await m_disconnectCompleted.async_receive(asio::use_awaitable);
}

ConnectionState ConnectionManager::state() const noexcept
{
    return m_state.load(std::memory_order_acquire);
}

void ConnectionManager::closeConnectingSocket() noexcept
{
    if (!m_connectingSocket)
    {
        return;
    }

    system::error_code ignored;
    m_connectingSocket->cancel(ignored);
    m_connectingSocket->close(ignored);
}

void ConnectionManager::connectionClosed()
{
    if (state() == ConnectionState::Connected ||
        state() == ConnectionState::Disconnecting)
    {
        setState(ConnectionState::Disconnected);
    }
}

void ConnectionManager::setState(ConnectionState state)
{
    auto const previousState = m_state.exchange(state, std::memory_order_acq_rel);

    if (previousState == ConnectionState::Disconnecting && state == ConnectionState::Disconnected)
    {
        m_disconnectCompleted.try_send(system::error_code{});
    }
}
} // namespace network
