#pragma once

#include "ClientRequestManager.hpp"
#include "ClientTypes.hpp"

#include <atomic>
#include <chrono>
#include <memory>
#include <optional>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace network
{
class ConnectionManager : public std::enable_shared_from_this<ConnectionManager>
{
public:
    ConnectionManager(
        asio::strand<asio::any_io_executor> strand,
        Config const& config,
        std::shared_ptr<ClientRequestManager> manager);

    ConnectionManager(ConnectionManager const&) = delete;
    ConnectionManager& operator=(ConnectionManager const&) = delete;
    ConnectionManager(ConnectionManager&&) = delete;
    ConnectionManager& operator=(ConnectionManager&&) = delete;

    asio::awaitable<void> connect(asio::ip::tcp::endpoint endpoint);
    asio::awaitable<void> disconnect();

    [[nodiscard]] ConnectionState state() const noexcept;

private:
    asio::awaitable<void> waitForDisconnected();
    void closeConnectingSocket() noexcept;
    void connectionClosed();
    void setState(ConnectionState state);

private:
    asio::strand<asio::any_io_executor> m_strand;
    std::chrono::steady_clock::duration m_connectTimeout;
    std::shared_ptr<ClientRequestManager> m_manager;

    std::atomic<ConnectionState> m_state{ConnectionState::Disconnected};
    std::optional<asio::ip::tcp::socket> m_connectingSocket;
    asio::steady_timer m_disconnectWaiters;
};
} // namespace network
