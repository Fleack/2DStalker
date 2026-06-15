#include "server/src/game/WorldState.hpp"

#include <optional>
#include <utility>

#include <nlohmann/json.hpp>

namespace s2d::game
{
WorldState WorldState::createInitial()
{
    return WorldState{TileMap{"bootstrap", 32, 18}};
}

WorldState::WorldState(TileMap map)
    : m_map{std::move(map)}
{
}

TileMap const& WorldState::map() const noexcept
{
    return m_map;
}

std::map<player_id, PlayerState> const& WorldState::players() const noexcept
{
    return m_players;
}

std::optional<PlayerState> WorldState::player(player_id id) const noexcept
{
    auto const it = m_players.find(id);
    if (it == m_players.end())
    {
        return std::nullopt;
    }

    return it->second;
}

player_id WorldState::addPlayer()
{
    if (m_nextPlayerId == std::numeric_limits<player_id>::max()) [[unlikely]]
    {
        throw std::runtime_error("Too many players");
    }
    auto const id = m_nextPlayerId++;
    m_players.emplace(id, PlayerState{id, m_spawnPosition});

    return id;
}

bool WorldState::removePlayer(player_id id)
{
    return m_players.erase(id) > 0;
}

bool WorldState::movePlayer(player_id id, Position delta)
{
    auto it = m_players.find(id);
    if (it == m_players.end())
    {
        return false;
    }

    auto& playerState = it->second;
    playerState.position.x += delta.x;
    playerState.position.y += delta.y;
    return true;
}

std::string WorldState::toJson() const
{
    nlohmann::json playersJson = nlohmann::json::array();
    for (auto const& player_state : players() | std::views::values)
    {
        auto const& player = player_state;
        playersJson.push_back({
            {"id", player.id},
            {"position", {{"x", player.position.x}, {"y", player.position.y}}},
        });
    }

    auto const& mapJson = map();
    nlohmann::json snapshot{
        {"world", mapJson.name},
        {"map", {{"width", mapJson.width}, {"height", mapJson.height}}},
        {"players", std::move(playersJson)},
    };

    return snapshot.dump();
}
} // namespace s2d::game
