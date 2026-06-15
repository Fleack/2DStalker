#pragma once

#include <compare>
#include <cstdint>
#include <string>

namespace s2d::game
{
using player_id = std::uint8_t;

struct Position
{
    int x{0};
    int y{0};

    auto operator<=>(Position const&) const noexcept = default;
};

struct TileMap
{
    std::string name;
    int width{0};
    int height{0};
};

struct PlayerState
{
    player_id id{0};
    Position position{};
};
} // namespace s2d::game
