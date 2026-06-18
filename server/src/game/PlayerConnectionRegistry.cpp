#include "server/src/game/PlayerConnectionRegistry.hpp"

namespace s2d::game
{
std::optional<player_id> PlayerConnectionRegistry::playerFor(connection_key connection) const
{
    auto const it = m_playersByConnection.find(connection);
    if (it == m_playersByConnection.end())
    {
        return std::nullopt;
    }

    return it->second;
}

void PlayerConnectionRegistry::bind(connection_key connection, player_id player)
{
    m_playersByConnection.insert_or_assign(connection, player);
}

void PlayerConnectionRegistry::unbind(connection_key connection)
{
    m_playersByConnection.erase(connection);
}
} // namespace s2d::game
