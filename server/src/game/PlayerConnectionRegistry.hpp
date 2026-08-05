#pragma once

#include "server/src/game/WorldTypes.hpp"

#include <cstdint>
#include <map>
#include <optional>

namespace s2d::game
{
using connection_key = std::uint16_t;

class PlayerConnectionRegistry
{
public:
    [[nodiscard]] std::optional<player_id> playerFor(connection_key connection) const;

    void bind(connection_key connection, player_id player);
    void unbind(connection_key connection);

private:
    std::map<connection_key, player_id> m_playersByConnection;
};
} // namespace s2d::game
