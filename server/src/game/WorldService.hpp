#pragma once

#include "server/src/game/PlayerConnectionRegistry.hpp"
#include "server/src/game/WorldAction.hpp"
#include "server/src/game/WorldState.hpp"

#include <string>

namespace s2d::game
{
class WorldService
{
public:
    WorldService();
    explicit WorldService(WorldState state);

    player_id connect(connection_key connection);
    void disconnect(connection_key connection);
    void apply(connection_key connection, WorldAction const& action);

    [[nodiscard]] std::string snapshotJson() const;
    [[nodiscard]] WorldState const& state() const noexcept;

private:
    void applyToPlayer(player_id player, MoveAction const& action);
    void applyToPlayer(player_id player, InteractAction const& action);
    void applyToPlayer(player_id player, WaitAction const& action);

private:
    WorldState m_state;
    PlayerConnectionRegistry m_connections;
};
} // namespace s2d::game
