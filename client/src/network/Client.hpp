#pragma once

#include "ClientRequestManager.hpp"
#include "shared/network/Connection.hpp"
#include "shared/protocol/message.pb.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

#include <asio/any_io_executor.hpp>
#include <asio/awaitable.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/address.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/steady_timer.hpp>
#include <asio/strand.hpp>

class Client : public std::enable_shared_from_this<Client>
{
public:
    struct Config
    {
        std::uint32_t maxMessageBytes{1024 * 1024};
        std::size_t maxQueuedMessages{1024};
        std::size_t maxPendingRequests{1024};
        std::chrono::steady_clock::duration requestTimeout{std::chrono::seconds{5}};
    };

    static std::shared_ptr<Client> create(asio::io_context& ctx);
    static std::shared_ptr<Client> create(asio::io_context& ctx, Config config);

    ~Client();

    Client(Client const&) = delete;
    Client& operator=(Client const&) = delete;

    Client(Client&&) = delete;
    Client& operator=(Client&&) = delete;

    asio::awaitable<void> connect(asio::ip::address ip, std::uint16_t port);
    void disconnect() noexcept;

    [[nodiscard]] bool isConnected() const noexcept;

    asio::awaitable<s2d::protocol::ServerMessage> send(s2d::protocol::ClientMessage message);

private:
    using connection_t = s2d::network::Connection<
        s2d::protocol::ServerMessage,
        s2d::protocol::ClientMessage>;

    enum class PendingState
    {
        waiting,
        completed,
        failed,
    };

    struct PendingRequest
    {
        explicit PendingRequest(asio::any_io_executor const& executor);

        asio::steady_timer timer;
        PendingState state{PendingState::waiting};
        std::optional<s2d::protocol::ServerMessage> response;
        std::string error;
    };

private:
    explicit Client(asio::io_context& ctx, Config config);

    asio::awaitable<void> connectImpl(asio::ip::address ip, std::uint16_t port);
    asio::awaitable<s2d::protocol::ServerMessage> sendImpl(s2d::protocol::ClientMessage message);

    void disconnectImpl(std::string_view reason) noexcept;

    asio::awaitable<void> handleMessage(
        s2d::network::connection_id connectionId,
        s2d::protocol::ServerMessage message);

    void onConnectionClosed(s2d::network::connection_id connectionId) noexcept;
    void onConnectionClosedImpl(s2d::network::connection_id connectionId) noexcept;

    void failPendingRequests(std::string_view reason) noexcept;

    [[nodiscard]] std::uint64_t nextRequestId() noexcept;

    static void validateConfig(Config const& config);

private:
    asio::io_context& m_io;
    asio::strand<asio::any_io_executor> m_strand;

    Config m_config;

    std::shared_ptr<connection_t> m_connection;

    std::unordered_map<std::uint64_t, std::unique_ptr<PendingRequest>> m_pendingRequests;

    std::uint64_t m_nextRequestId{0};

    bool m_connecting{false};
};
