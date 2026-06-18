#include "server/src/game/WorldService.hpp"

#include "shared/logger/logger.hpp"

#include <utility>
#include <variant>

namespace s2d::game
{
WorldService::WorldService()
    : WorldService{WorldState::createInitial()}
{
}

WorldService::WorldService(WorldState state)
    : m_state{std::move(state)}
{
}

player_id WorldService::connect(connection_key connection)
{
    if (auto const existingPlayer = m_connections.playerFor(connection))
    {
        return *existingPlayer;
    }

    auto const player = m_state.addPlayer();
    m_connections.bind(connection, player);
    return player;
}

void WorldService::disconnect(connection_key connection)
{
    auto const player = m_connections.playerFor(connection);
    if (!player.has_value())
    {
        LOG(warn, "Trying to disconnect non-existing player by connection=[{}]", connection);
        return;
    }

    m_connections.unbind(connection);
    m_state.removePlayer(*player);
}

void WorldService::apply(connection_key connection, WorldAction const& action)
{
    auto const player = m_connections.playerFor(connection);
    if (!player.has_value())
    {
        LOG(warn, "Received action for non-existing player by connection=[{}]", connection);
        return;
    }
    std::visit([this, player](auto const& typedAction) { applyToPlayer(player.value(), typedAction); }, action);
}

std::string WorldService::snapshotJson() const
{
    return m_state.toJson();
}

WorldState const& WorldService::state() const noexcept
{
    return m_state;
}

void WorldService::applyToPlayer(player_id player, MoveAction const& action)
{
    m_state.movePlayer(player, action.delta);
}

void WorldService::applyToPlayer(player_id, InteractAction const&)
{
}

void WorldService::applyToPlayer(player_id, WaitAction const&)
{
}
} // namespace s2d::game
