#pragma once

#include "server/src/game/WorldTypes.hpp"

#include <map>
#include <optional>

namespace s2d::game
{
class WorldState
{
public:
    static WorldState createInitial();

    explicit WorldState(TileMap map);

    [[nodiscard]] TileMap const& map() const noexcept;
    [[nodiscard]] std::map<player_id, PlayerState> const& players() const noexcept;
    [[nodiscard]] std::optional<PlayerState> player(player_id id) const noexcept;

    player_id addPlayer();
    bool removePlayer(player_id id);
    bool movePlayer(player_id id, Position delta);

    [[nodiscard]] std::string toJson() const;

private:
    std::map<player_id, PlayerState> m_players;
    TileMap m_map;
    Position m_spawnPosition{};
    player_id m_nextPlayerId{1};
};
} // namespace s2d::game
