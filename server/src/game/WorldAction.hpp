#pragma once

#include "server/src/game/WorldTypes.hpp"

#include <variant>

namespace s2d::game
{
struct MoveAction
{
    Position delta{};
};

struct InteractAction
{
};

struct WaitAction
{
};

using WorldAction = std::variant<MoveAction, InteractAction, WaitAction>;
} // namespace s2d::game
