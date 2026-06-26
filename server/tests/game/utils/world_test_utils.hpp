#pragma once

#include "server/src/game/WorldService.hpp"
#include "server/src/game/WorldState.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

namespace s2d::test::game
{

inline nlohmann::json parse_world_json(s2d::game::WorldState const& world)
{
    return nlohmann::json::parse(world.toJson());
}

inline nlohmann::json parse_snapshot(s2d::game::WorldService const& service)
{
    return nlohmann::json::parse(service.snapshotJson());
}

inline void require_no_player(s2d::game::WorldService const& service, s2d::game::player_id player)
{
    REQUIRE_FALSE(service.state().player(player).has_value());
}

inline s2d::game::PlayerState require_player(
    s2d::game::WorldService const& service,
    s2d::game::player_id player)
{
    auto const player_state = service.state().player(player);

    REQUIRE(player_state.has_value());
    REQUIRE(player_state->id == player);

    return player_state.value();
}

inline void require_position(s2d::game::Position const& actual, s2d::game::Position const& expected)
{
    REQUIRE(actual.x == expected.x);
    REQUIRE(actual.y == expected.y);
}

inline void require_json_player(
    nlohmann::json const& json,
    s2d::game::player_id expected_id,
    s2d::game::Position const& expected_position)
{
    REQUIRE(json.at("id").get<s2d::game::player_id>() == expected_id);
    REQUIRE(json.at("position").at("x").get<decltype(expected_position.x)>() == expected_position.x);
    REQUIRE(json.at("position").at("y").get<decltype(expected_position.y)>() == expected_position.y);
}

inline s2d::game::WorldAction make_move_action(s2d::game::Position delta)
{
    return s2d::game::WorldAction{s2d::game::MoveAction{delta}};
}

inline s2d::game::WorldAction make_interact_action()
{
    return s2d::game::WorldAction{s2d::game::InteractAction{}};
}

inline s2d::game::WorldAction make_wait_action()
{
    return s2d::game::WorldAction{s2d::game::WaitAction{}};
}

} // namespace s2d::test::game
