#include "ClientRequestManager.hpp"

#include "shared/logger/logger.hpp"

#include <limits>
#include <ranges>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <boost/asio/dispatch.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/use_awaitable.hpp>

namespace network
{
ClientRequestManager::PendingRequest::PendingRequest(asio::any_io_executor const& executor)
    : timer{executor}
{
}

ClientRequestManager::ClientRequestManager(asio::strand<asio::any_io_executor> strand, Config const& config)
    : m_strand{std::move(strand)}
    , m_maxMessageBytes{config.maxMessageBytes}
    , m_maxQueuedMessages{config.maxQueuedMessages}
    , m_maxPendingRequests{config.maxPendingRequests}
    , m_requestTimeout{config.requestTimeout}
{
}

void ClientRequestManager::start(asio::ip::tcp::socket socket, close_handler_t onClosed)
{
    if (m_connection)
    {
        throw std::logic_error{"Client connection is already active"};
    }

    auto weak = weak_from_this();
    static constexpr auto connectionId = s2d::network::connection_id{0};
    auto connection = connection_t::create(
        connectionId,
        std::move(socket),
        connection_t::Config{
            .maxMessageBytes = m_maxMessageBytes,
            .maxQueuedMessages = m_maxQueuedMessages,
        },
        [weak](s2d::protocol::ServerMessage message) -> asio::awaitable<void> {
            if (auto self = weak.lock())
            {
                co_await self->receive(std::move(message));
            }
        },
        [weak]() noexcept {
            try
            {
                if (auto self = weak.lock())
                {
                    asio::post(
                        self->m_strand,
                        [weak] {
                            if (auto locked = weak.lock())
                            {
                                locked->connectionClosed();
                            }
                        });
                }
            }
            catch (std::exception const& error)
            {
                LOG(err, "Failed to schedule client close handling: {}", error.what());
            }
        });

    m_connection = std::move(connection);
    m_onClosed = std::move(onClosed);
    m_connection->start();
}

void ClientRequestManager::stop(std::string_view reason) noexcept
{
    if (!m_connection)
    {
        return;
    }

    failPendingRequests(reason);
    m_connection->stop();
}

asio::awaitable<s2d::protocol::PongResponse> ClientRequestManager::sendRequest(s2d::protocol::PingRequest request)
{
    [[maybe_unused]] auto selfGuard = shared_from_this();
    co_await asio::dispatch(m_strand, asio::use_awaitable);

    s2d::protocol::ClientMessage message;
    *message.mutable_ping() = std::move(request);

    auto response = co_await send(std::move(message));
    if (response.status() != s2d::protocol::STATUS_OK)
    {
        throw std::runtime_error{response.error().message()};
    }
    if (!response.has_pong())
    {
        throw std::runtime_error{"Ping response has unexpected payload"};
    }

    co_return std::move(*response.mutable_pong());
}

asio::awaitable<s2d::protocol::StateSnapshotResponse> ClientRequestManager::sendRequest(s2d::protocol::StateSnapshotRequest request)
{
    [[maybe_unused]] auto selfGuard = shared_from_this();
    co_await asio::dispatch(m_strand, asio::use_awaitable);

    s2d::protocol::ClientMessage message;
    *message.mutable_state_snapshot() = std::move(request);

    auto response = co_await send(std::move(message));
    if (response.status() != s2d::protocol::STATUS_OK)
    {
        throw std::runtime_error{response.error().message()};
    }
    if (!response.has_state_snapshot())
    {
        throw std::runtime_error{"State snapshot response has unexpected payload"};
    }

    co_return std::move(*response.mutable_state_snapshot());
}

asio::awaitable<s2d::protocol::ServerMessage> ClientRequestManager::send(s2d::protocol::ClientMessage message)
{
    if (!m_connection)
    {
        throw std::runtime_error{"Client is not connected"};
    }

    if (m_pendingRequests.size() >= m_maxPendingRequests)
    {
        throw std::runtime_error{"Too many pending client requests"};
    }

    auto const requestId = nextRequestId();
    message.set_request_id(requestId);

    auto pending = std::make_unique<PendingRequest>(m_strand);
    auto* pendingRaw = pending.get();
    pending->timer.expires_after(m_requestTimeout);

    m_pendingRequests.emplace(requestId, std::move(pending));

    m_connection->send(std::move(message));

    system::error_code error;
    co_await pendingRaw->timer.async_wait(asio::redirect_error(asio::use_awaitable, error));

    auto pendingIt = m_pendingRequests.find(requestId);
    if (pendingIt == m_pendingRequests.end() || pendingIt->second.get() != pendingRaw)
    {
        throw std::runtime_error{"Pending request state was lost"};
    }

    auto completed = std::move(pendingIt->second);
    m_pendingRequests.erase(pendingIt);

    if (!error)
    {
        throw std::runtime_error{"Request timed out"};
    }

    if (error != asio::error::operation_aborted)
    {
        throw std::system_error{error, "Pending request wait failed"};
    }

    if (completed->state == PendingState::Failed)
    {
        throw std::runtime_error{completed->error};
    }

    if (completed->state != PendingState::Completed || !completed->response)
    {
        throw std::runtime_error{"Pending request was cancelled without response"};
    }

    co_return std::move(*completed->response);
}

asio::awaitable<void> ClientRequestManager::receive(s2d::protocol::ServerMessage message)
{
    [[maybe_unused]] auto selfGuard = shared_from_this();
    co_await asio::dispatch(m_strand, asio::use_awaitable);

    auto const requestId = message.request_id();
    auto it = m_pendingRequests.find(requestId);
    if (it == m_pendingRequests.end())
    {
        LOG(warn, "Received unexpected response with request_id {}", requestId);
        co_return;
    }

    auto& pending = *it->second;
    if (pending.state != PendingState::Waiting)
    {
        LOG(warn, "Received response for completed request_id {}", requestId);
        co_return;
    }

    pending.state = PendingState::Completed;
    pending.response = std::move(message);
    pending.timer.cancel();
}

void ClientRequestManager::connectionClosed() noexcept
{
    m_connection.reset();
    failPendingRequests("Connection closed before response was received");

    if (!m_onClosed)
        return;

    try
    {
        m_onClosed();
    }
    catch (std::exception const& error)
    {
        LOG(err, "Client close handler failed: {}", error.what());
    }
    m_onClosed = {};
}

void ClientRequestManager::failPendingRequests(std::string_view reason) noexcept
{
    for (auto& pending : m_pendingRequests | std::views::values)
    {
        if (pending->state == PendingState::Waiting)
        {
            pending->state = PendingState::Failed;
            pending->error = reason;
            pending->timer.cancel();
        }
    }
}

std::uint64_t ClientRequestManager::nextRequestId() noexcept
{
    return ++m_nextRequestId;
}

} // namespace network
