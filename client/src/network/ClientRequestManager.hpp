#pragma once

#include "ClientTypes.hpp"
#include "shared/network/Connection.hpp"
#include "shared/protocol/message.pb.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>

namespace network
{
using namespace boost;

class ClientRequestManager : public std::enable_shared_from_this<ClientRequestManager>
{
public:
    using connection_t = s2d::network::Connection<s2d::protocol::ServerMessage, s2d::protocol::ClientMessage>;
    using close_handler_t = std::move_only_function<void()>;

    ClientRequestManager(asio::strand<asio::any_io_executor> strand, Config const& config);

    ClientRequestManager(ClientRequestManager const&) = delete;
    ClientRequestManager& operator=(ClientRequestManager const&) = delete;
    ClientRequestManager(ClientRequestManager&&) = delete;
    ClientRequestManager& operator=(ClientRequestManager&&) = delete;

    void start(asio::ip::tcp::socket socket, close_handler_t onClosed);
    void stop(std::string_view reason) noexcept;

    asio::awaitable<s2d::protocol::PongResponse> sendRequest(s2d::protocol::PingRequest request);
    asio::awaitable<s2d::protocol::StateSnapshotResponse> sendRequest(s2d::protocol::StateSnapshotRequest request);

private:
    enum class PendingState
    {
        Waiting,
        Completed,
        Failed,
    };

    struct PendingRequest
    {
        explicit PendingRequest(asio::any_io_executor const& executor);

        asio::steady_timer timer;
        PendingState state{PendingState::Waiting};
        std::optional<s2d::protocol::ServerMessage> response;
        std::string error;
    };

    asio::awaitable<s2d::protocol::ServerMessage> send(s2d::protocol::ClientMessage message);
    asio::awaitable<void> receive(s2d::protocol::ServerMessage message);

    void connectionClosed() noexcept;
    void failPendingRequests(std::string_view reason) noexcept;

    [[nodiscard]] std::uint64_t nextRequestId() noexcept;

private:
    asio::strand<asio::any_io_executor> m_strand;

    std::uint32_t m_maxMessageBytes;
    std::size_t m_maxQueuedMessages;
    std::size_t m_maxPendingRequests;
    std::chrono::steady_clock::duration m_requestTimeout;

    std::uint64_t m_nextRequestId{0};

    std::shared_ptr<connection_t> m_connection;
    close_handler_t m_onClosed;
    std::unordered_map<std::uint64_t, std::unique_ptr<PendingRequest>> m_pendingRequests;
};
} // namespace network
