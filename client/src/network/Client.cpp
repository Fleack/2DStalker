#include "Client.hpp"

#include "shared/logger/logger.hpp"

#include <limits>
#include <stdexcept>
#include <system_error>
#include <utility>

#include <asio/dispatch.hpp>
#include <asio/error.hpp>
#include <asio/post.hpp>
#include <asio/redirect_error.hpp>
#include <asio/use_awaitable.hpp>

Client::PendingRequest::PendingRequest(asio::any_io_executor const& executor)
    : timer{executor}
{
}

std::shared_ptr<Client> Client::create(asio::io_context& ctx)
{
    return create(ctx, Config{});
}

std::shared_ptr<Client> Client::create(asio::io_context& ctx, Config config)
{
    validateConfig(config);

    return std::shared_ptr<Client>{
        new Client{ctx, config}};
}

Client::Client(asio::io_context& ctx, Config config)
    : m_io{ctx}
    , m_strand{asio::make_strand(ctx)}
    , m_config{config}
{
}

Client::~Client()
{
    if (m_connection)
    {
        m_connection->stop();
    }
}

asio::awaitable<void> Client::connect(asio::ip::address ip, std::uint16_t port)
{
    auto self = shared_from_this();

    co_await asio::dispatch(m_strand, asio::use_awaitable);
    co_await self->connectImpl(ip, port);
}

void Client::disconnect() noexcept
{
    try
    {
        auto weak = weak_from_this();

        asio::post(
            m_strand,
            [weak] {
                if (auto self = weak.lock())
                {
                    self->disconnectImpl("client disconnected");
                }
            });
    }
    catch (...)
    {
    }
}

bool Client::isConnected() const noexcept
{
    return static_cast<bool>(m_connection);
}

asio::awaitable<s2d::protocol::ServerMessage> Client::send(s2d::protocol::ClientMessage message)
{
    auto self = shared_from_this();

    co_await asio::dispatch(m_strand, asio::use_awaitable);
    co_return co_await self->sendImpl(std::move(message));
}

asio::awaitable<void> Client::connectImpl(asio::ip::address ip, std::uint16_t port)
{
    if (m_connection)
    {
        throw std::runtime_error{"Client is already connected"};
    }

    if (m_connecting)
    {
        throw std::runtime_error{"Client is already connecting"};
    }

    static constexpr s2d::network::connection_id clientConnectionId{1};

    auto endpoint = asio::ip::tcp::endpoint{ip, port};
    auto socket = std::make_shared<asio::ip::tcp::socket>(m_strand);

    m_connecting = true;

    asio::error_code ec;
    co_await socket->async_connect(
        endpoint,
        asio::redirect_error(asio::use_awaitable, ec));

    m_connecting = false;

    if (ec)
    {
        LOG(err,
            "Failed to connect to server[{}:{}]: {}",
            endpoint.address().to_string(),
            endpoint.port(),
            ec.message());

        throw std::system_error{ec, "Failed to connect to server"};
    }

    auto weak = weak_from_this();

    auto messageHandler =
        [weak](
            s2d::network::connection_id connectionId,
            s2d::protocol::ServerMessage message) -> asio::awaitable<void> {
        if (auto self = weak.lock())
        {
            co_await self->handleMessage(
                connectionId,
                std::move(message));
        }
    };

    auto closeHandler =
        [weak](s2d::network::connection_id connectionId) noexcept {
            if (auto self = weak.lock())
            {
                self->onConnectionClosed(connectionId);
            }
        };

    m_connection = connection_t::create(
        clientConnectionId,
        std::move(*socket),
        connection_t::Config{
            .maxMessageBytes = m_config.maxMessageBytes,
            .maxQueuedMessages = m_config.maxQueuedMessages,
        },
        std::move(messageHandler),
        std::move(closeHandler));

    m_connection->start();
}

asio::awaitable<s2d::protocol::ServerMessage> Client::sendImpl(
    s2d::protocol::ClientMessage message)
{
    if (!m_connection)
    {
        throw std::runtime_error{"Client is not connected"};
    }

    if (m_pendingRequests.size() >= m_config.maxPendingRequests)
    {
        throw std::runtime_error{"Too many pending client requests"};
    }

    auto const requestId = nextRequestId();
    message.set_request_id(requestId);

    auto pending = std::make_unique<PendingRequest>(m_strand);
    auto* pendingRaw = pending.get();

    pending->timer.expires_after(m_config.requestTimeout);

    auto [it, inserted] = m_pendingRequests.emplace(requestId, std::move(pending));
    if (!inserted)
    {
        throw std::runtime_error{"Generated duplicate request_id"};
    }

    m_connection->send(std::move(message));

    asio::error_code ec;
    co_await pendingRaw->timer.async_wait(
        asio::redirect_error(asio::use_awaitable, ec));

    auto pendingIt = m_pendingRequests.find(requestId);
    if (pendingIt == m_pendingRequests.end() || pendingIt->second.get() != pendingRaw)
    {
        throw std::runtime_error{"Pending request state was lost"};
    }

    auto completedRequest = std::move(pendingIt->second);
    m_pendingRequests.erase(pendingIt);

    if (!ec)
    {
        throw std::runtime_error{"Request timed out"};
    }

    if (ec != asio::error::operation_aborted)
    {
        throw std::system_error{ec, "Pending request wait failed"};
    }

    if (completedRequest->state == PendingState::failed)
    {
        throw std::runtime_error{completedRequest->error};
    }

    if (completedRequest->state != PendingState::completed ||
        !completedRequest->response.has_value())
    {
        throw std::runtime_error{"Pending request was cancelled without response"};
    }

    co_return std::move(*completedRequest->response);
}

void Client::disconnectImpl(std::string_view reason) noexcept
{
    m_connecting = false;

    auto connection = std::exchange(m_connection, nullptr);
    if (connection == nullptr)
    {
        LOG(warn, "Client is already disconnected");
        return;
    }

    failPendingRequests(reason);
    connection->stop();
}

asio::awaitable<void> Client::handleMessage(
    s2d::network::connection_id,
    s2d::protocol::ServerMessage message)
{
    auto self = shared_from_this();

    co_await asio::dispatch(m_strand, asio::use_awaitable);

    auto const requestId = message.request_id();
    auto it = m_pendingRequests.find(requestId);

    if (it == m_pendingRequests.end())
    {
        LOG(warn, "Received unexpected response with request_id {}", requestId);
        co_return;
    }

    auto& pending = *it->second;

    if (pending.state != PendingState::waiting)
    {
        LOG(warn, "Received response for already completed request_id {}", requestId);
        co_return;
    }

    pending.state = PendingState::completed;
    pending.response = std::move(message);

    asio::error_code ignored;
    pending.timer.cancel(ignored);
}

void Client::onConnectionClosed(s2d::network::connection_id connectionId) noexcept
{
    try
    {
        auto weak = weak_from_this();

        asio::post(
            m_strand,
            [weak, connectionId] {
                if (auto self = weak.lock())
                {
                    self->onConnectionClosedImpl(connectionId);
                }
            });
    }
    catch (...)
    {
    }
}

void Client::onConnectionClosedImpl(s2d::network::connection_id) noexcept
{
    m_connection.reset();

    failPendingRequests("Connection closed before response was received");
}

void Client::failPendingRequests(std::string_view reason) noexcept
{
    for (auto& pending : m_pendingRequests | std::views::values)
    {
        if (pending->state != PendingState::waiting)
        {
            continue;
        }

        pending->state = PendingState::failed;
        pending->error = reason;

        asio::error_code ignored;
        pending->timer.cancel(ignored);
    }
}

std::uint64_t Client::nextRequestId() noexcept
{
    if (m_nextRequestId == std::numeric_limits<std::uint64_t>::max())
    {
        m_nextRequestId = 0;
    }

    ++m_nextRequestId;

    if (m_nextRequestId == 0)
    {
        ++m_nextRequestId;
    }

    return m_nextRequestId;
}

void Client::validateConfig(Config const& config)
{
    if (config.maxMessageBytes == 0)
    {
        throw std::invalid_argument{"Client maxMessageBytes must be greater than zero"};
    }

    if (config.maxQueuedMessages == 0)
    {
        throw std::invalid_argument{"Client maxQueuedMessages must be greater than zero"};
    }

    if (config.maxPendingRequests == 0)
    {
        throw std::invalid_argument{"Client maxPendingRequests must be greater than zero"};
    }

    if (config.requestTimeout <= std::chrono::steady_clock::duration::zero())
    {
        throw std::invalid_argument{"Client requestTimeout must be greater than zero"};
    }
}
