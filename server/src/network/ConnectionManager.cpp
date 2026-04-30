#include "ConnectionManager.hpp"

#include "shared/logger/logger.hpp"

#include <ranges>
#include <utility>

namespace s2d::network
{

// --- public ---

std::shared_ptr<Connection> ConnectionManager::create(asio::ip::tcp::socket&& socket, IMessageHandler& handler, std::uint32_t max_message_bytes)
{
    if (m_stopped)
    {
        LOG(err, "Failed to create connection: ConnectionManager is stopped");
        return {};
    }

    auto const connectionId = ++m_nextConnectionId;

    auto connection = std::make_shared<Connection>(
        connectionId,
        std::move(socket),
        handler,
        max_message_bytes,
        [this](connection_id id) { remove(id); });
    m_connections.emplace(connectionId, connection);

    return connection;
}

bool ConnectionManager::remove(connection_id id)
{
    if (m_stopped)
    {
        LOG(err, "Failed to remove connection: ConnectionManager is stopped");
        return false;
    }

    if (m_connections.erase(id) == 0)
    {
        LOG(debug, "Connection[id={}] not found", id.id);
        return false;
    }

    LOG(debug, "Connection[id={}] removed", id.id);
    return true;
}

std::shared_ptr<Connection> ConnectionManager::get(connection_id id) const
{
    if (m_stopped)
    {
        LOG(err, "Failed to get connection: ConnectionManager is stopped");
        return {};
    }

    auto const it = m_connections.find(id);
    if (it == m_connections.end())
    {
        return {};
    }

    return it->second;
}

asio::awaitable<void> ConnectionManager::send(connection_id id, protocol::ServerMessage const& message) const
{
    if (m_stopped)
    {
        LOG(err, "Failed to send message: ConnectionManager is stopped");
        co_return;
    }

    if (auto const connection = get(id))
    {
        co_await connection->send(message);
    }
}

asio::awaitable<void> ConnectionManager::broadcast(protocol::ServerMessage const& message) const
{
    if (m_stopped)
    {
        LOG(err, "Failed to broadcast message: ConnectionManager is stopped");
        co_return;
    }

    auto const snapshot = makeSnapshot();

    for (auto const& connection : snapshot)
    {
        co_await connection->send(message);
    }
}

void ConnectionManager::stopAll() noexcept
{
    if (std::exchange(m_stopped, true))
    {
        return;
    }

    auto const snapshot = makeSnapshot();
    m_connections.clear();

    for (auto const& connection : snapshot)
    {
        connection->stop();
    }
}

// --- private ---

std::vector<std::shared_ptr<Connection>> ConnectionManager::makeSnapshot() const
{
    auto values = m_connections | std::views::values;
    return std::ranges::to<std::vector<std::shared_ptr<Connection>>>(values);
}

} // namespace s2d::network
