#include "server/src/game/WorldState.hpp"
#include "utils/game_fixtures.hpp"
#include "utils/world_test_utils.hpp"

#include <string>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

namespace s2d::game
{
using s2d::test::game::parse_world_json;
using s2d::test::game::require_json_player;

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::createInitial creates bootstrap empty world",
    "[WorldState]")
{
    SECTION("creates expected map")
    {
        REQUIRE(world.map().name == "bootstrap");
        REQUIRE(world.map().width == 32);
        REQUIRE(world.map().height == 18);
    }

    SECTION("has no players")
    {
        REQUIRE(world.players().empty());
        REQUIRE_FALSE(world.player(player_id{1}).has_value());
    }

    SECTION("serializes empty world")
    {
        auto const json = parse_world_json(world);

        REQUIRE(json.at("world") == "bootstrap");
        REQUIRE(json.at("map").at("width") == 32);
        REQUIRE(json.at("map").at("height") == 18);
        REQUIRE(json.at("players").is_array());
        REQUIRE(json.at("players").empty());
    }
}

TEST_CASE("WorldState can be created with custom map", "[WorldState]")
{
    constexpr std::string_view mapName = "test-map";
    constexpr int mapWidth = 10;
    constexpr int mapHeight = 20;
    auto const world = WorldState{TileMap{std::string{mapName}, mapWidth, mapHeight}};

    REQUIRE(world.map().name == mapName);
    REQUIRE(world.map().width == mapWidth);
    REQUIRE(world.map().height == mapHeight);
    REQUIRE(world.players().empty());

    auto const json = parse_world_json(world);

    REQUIRE(json.at("world") == mapName);
    REQUIRE(json.at("map").at("width") == mapWidth);
    REQUIRE(json.at("map").at("height") == mapHeight);
    REQUIRE(json.at("players").empty());
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::player returns nullopt for unknown player",
    "[WorldState]")
{
    REQUIRE_FALSE(world.player(player_id{1}).has_value());
    REQUIRE_FALSE(world.player(player_id{42}).has_value());
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::addPlayer adds players with unique sequential ids",
    "[WorldState]")
{
    auto const firstId = world.addPlayer();
    auto const secondId = world.addPlayer();

    REQUIRE(firstId == player_id{1});
    REQUIRE(secondId == player_id{2});
    REQUIRE(world.players().size() == 2);

    auto const firstPlayer = world.player(firstId);
    auto const secondPlayer = world.player(secondId);

    REQUIRE(firstPlayer.has_value());
    REQUIRE(secondPlayer.has_value());

    REQUIRE(firstPlayer->id == firstId);
    REQUIRE(secondPlayer->id == secondId);

    // Оба игрока появляются в одной spawn-позиции.
    REQUIRE(secondPlayer->position.x == firstPlayer->position.x);
    REQUIRE(secondPlayer->position.y == firstPlayer->position.y);

    REQUIRE(world.players().contains(firstId));
    REQUIRE(world.players().contains(secondId));
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::removePlayer removes existing player",
    "[WorldState]")
{
    auto const id = world.addPlayer();

    REQUIRE(world.player(id).has_value());
    REQUIRE(world.players().size() == 1);

    REQUIRE(world.removePlayer(id));

    REQUIRE(world.players().empty());
    REQUIRE_FALSE(world.player(id).has_value());
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::removePlayer returns false for unknown player",
    "[WorldState]")
{
    SECTION("empty world")
    {
        REQUIRE_FALSE(world.removePlayer(player_id{1}));
        REQUIRE(world.players().empty());
    }

    SECTION("non-empty world")
    {
        auto const existingId = world.addPlayer();
        auto const unknownId = static_cast<player_id>(existingId + 1);

        REQUIRE_FALSE(world.removePlayer(unknownId));

        REQUIRE(world.players().size() == 1);
        REQUIRE(world.player(existingId).has_value());
    }
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::removePlayer does not reuse removed ids",
    "[WorldState]")
{
    auto const firstId = world.addPlayer();

    REQUIRE(world.removePlayer(firstId));

    auto const secondId = world.addPlayer();

    REQUIRE(firstId == player_id{1});
    REQUIRE(secondId == player_id{2});
    REQUIRE_FALSE(world.player(firstId).has_value());
    REQUIRE(world.player(secondId).has_value());
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::movePlayer returns false for unknown player",
    "[WorldState]")
{
    REQUIRE_FALSE(world.movePlayer(player_id{1}, Position{1, 2}));
    REQUIRE(world.players().empty());
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::movePlayer moves existing player",
    "[WorldState]")
{
    auto const id = world.addPlayer();
    auto const before = world.player(id);

    REQUIRE(before.has_value());

    Position const delta{3, -2};

    REQUIRE(world.movePlayer(id, delta));

    auto const after = world.player(id);

    REQUIRE(after.has_value());
    REQUIRE(after->id == id);
    REQUIRE(after->position.x == before->position.x + delta.x);
    REQUIRE(after->position.y == before->position.y + delta.y);
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::movePlayer can move player multiple times",
    "[WorldState]")
{
    auto const id = world.addPlayer();
    auto const initial = world.player(id);

    REQUIRE(initial.has_value());

    REQUIRE(world.movePlayer(id, Position{10, 5}));
    REQUIRE(world.movePlayer(id, Position{-3, 7}));
    REQUIRE(world.movePlayer(id, Position{0, -2}));

    auto const player = world.player(id);

    REQUIRE(player.has_value());
    REQUIRE(player->position.x == initial->position.x + 7);
    REQUIRE(player->position.y == initial->position.y + 10);
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::movePlayer does not affect other players",
    "[WorldState]")
{
    auto const firstId = world.addPlayer();
    auto const secondId = world.addPlayer();

    auto const firstBefore = world.player(firstId);
    auto const secondBefore = world.player(secondId);

    REQUIRE(firstBefore.has_value());
    REQUIRE(secondBefore.has_value());

    REQUIRE(world.movePlayer(firstId, Position{4, 9}));

    auto const firstAfter = world.player(firstId);
    auto const secondAfter = world.player(secondId);

    REQUIRE(firstAfter.has_value());
    REQUIRE(secondAfter.has_value());

    REQUIRE(firstAfter->position.x == firstBefore->position.x + 4);
    REQUIRE(firstAfter->position.y == firstBefore->position.y + 9);

    REQUIRE(secondAfter->position.x == secondBefore->position.x);
    REQUIRE(secondAfter->position.y == secondBefore->position.y);
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::toJson serializes players in id order",
    "[WorldState]")
{
    auto const firstId = world.addPlayer();
    auto const secondId = world.addPlayer();

    REQUIRE(world.movePlayer(firstId, Position{5, 1}));
    REQUIRE(world.movePlayer(secondId, Position{-2, 3}));

    auto const firstPlayer = world.player(firstId);
    auto const secondPlayer = world.player(secondId);

    REQUIRE(firstPlayer.has_value());
    REQUIRE(secondPlayer.has_value());

    auto const json = parse_world_json(world);

    REQUIRE(json.at("world") == "bootstrap");
    REQUIRE(json.at("map").at("width") == 32);
    REQUIRE(json.at("map").at("height") == 18);

    auto const& players = json.at("players");

    REQUIRE(players.is_array());
    REQUIRE(players.size() == 2);

    // std::map хранит игроков по id, значит сериализация стабильна по возрастанию id.
    require_json_player(players.at(0), firstId, firstPlayer->position);
    require_json_player(players.at(1), secondId, secondPlayer->position);
}

TEST_CASE_METHOD(
    s2d::test::game::world_state_fixture,
    "WorldState::toJson does not serialize removed players",
    "[WorldState]")
{
    auto const removedId = world.addPlayer();
    auto const aliveId = world.addPlayer();

    REQUIRE(world.removePlayer(removedId));
    REQUIRE(world.movePlayer(aliveId, Position{7, 8}));

    auto const alivePlayer = world.player(aliveId);

    REQUIRE(alivePlayer.has_value());

    auto const json = parse_world_json(world);
    auto const& players = json.at("players");

    REQUIRE(players.is_array());
    REQUIRE(players.size() == 1);

    require_json_player(players.at(0), aliveId, alivePlayer->position);
}

} // namespace s2d::game
