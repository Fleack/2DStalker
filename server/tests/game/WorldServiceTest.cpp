#include "server/src/game/WorldService.hpp"
#include "utils/game_fixtures.hpp"
#include "utils/game_test_constants.hpp"
#include "utils/world_test_utils.hpp"

#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

namespace s2d::game
{
using s2d::test::game::connection_1;
using s2d::test::game::connection_2;
using s2d::test::game::connection_3;
using s2d::test::game::make_interact_action;
using s2d::test::game::make_move_action;
using s2d::test::game::make_wait_action;
using s2d::test::game::parse_snapshot;
using s2d::test::game::require_json_player;
using s2d::test::game::require_no_player;
using s2d::test::game::require_player;
using s2d::test::game::require_position;

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService default constructor creates initial empty world",
    "[WorldService]")
{
    REQUIRE(service.state().map().name == "bootstrap");
    REQUIRE(service.state().map().width == 32);
    REQUIRE(service.state().map().height == 18);

    REQUIRE(service.state().players().empty());

    auto const json = parse_snapshot(service);

    REQUIRE(json.at("world") == "bootstrap");
    REQUIRE(json.at("map").at("width") == 32);
    REQUIRE(json.at("map").at("height") == 18);
    REQUIRE(json.at("players").is_array());
    REQUIRE(json.at("players").empty());
}

TEST_CASE("WorldService can be constructed with custom WorldState", "[WorldService]")
{
    auto state = WorldState{TileMap{"custom-map", 100, 50}};
    auto service = WorldService{std::move(state)};

    REQUIRE(service.state().map().name == "custom-map");
    REQUIRE(service.state().map().width == 100);
    REQUIRE(service.state().map().height == 50);
    REQUIRE(service.state().players().empty());

    auto const json = parse_snapshot(service);

    REQUIRE(json.at("world") == "custom-map");
    REQUIRE(json.at("map").at("width") == 100);
    REQUIRE(json.at("map").at("height") == 50);
    REQUIRE(json.at("players").empty());
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::connect creates player for new connection",
    "[WorldService]")
{
    auto const player = service.connect(connection_1);

    REQUIRE(player == player_id{1});
    REQUIRE(service.state().players().size() == 1);

    auto const playerState = require_player(service, player);

    REQUIRE(playerState.id == player);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::connect returns existing player for already connected connection",
    "[WorldService]")
{
    auto const firstConnect = service.connect(connection_1);
    auto const secondConnect = service.connect(connection_1);

    REQUIRE(secondConnect == firstConnect);
    REQUIRE(service.state().players().size() == 1);

    require_player(service, firstConnect);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::connect creates independent players for different connections",
    "[WorldService]")
{
    auto const playerA = service.connect(connection_1);
    auto const playerB = service.connect(connection_2);
    auto const playerC = service.connect(connection_3);

    REQUIRE(playerA == player_id{1});
    REQUIRE(playerB == player_id{2});
    REQUIRE(playerC == player_id{3});

    REQUIRE(service.state().players().size() == 3);

    require_player(service, playerA);
    require_player(service, playerB);
    require_player(service, playerC);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::disconnect unknown connection is no-op",
    "[WorldService]")
{
    service.disconnect(connection_1);

    REQUIRE(service.state().players().empty());

    auto const json = parse_snapshot(service);

    REQUIRE(json.at("players").empty());
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::disconnect removes connected player",
    "[WorldService]")
{
    auto const player = service.connect(connection_1);

    REQUIRE(service.state().players().size() == 1);
    require_player(service, player);

    service.disconnect(connection_1);

    REQUIRE(service.state().players().empty());
    require_no_player(service, player);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::disconnect removes only player for requested connection",
    "[WorldService]")
{
    auto const playerA = service.connect(connection_1);
    auto const playerB = service.connect(connection_2);

    service.disconnect(connection_1);

    REQUIRE(service.state().players().size() == 1);

    require_no_player(service, playerA);
    require_player(service, playerB);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::disconnect can be called repeatedly for same connection",
    "[WorldService]")
{
    auto const player = service.connect(connection_1);

    service.disconnect(connection_1);
    service.disconnect(connection_1);
    service.disconnect(connection_1);

    REQUIRE(service.state().players().empty());
    require_no_player(service, player);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService reconnect after disconnect creates new player id",
    "[WorldService]")
{
    auto const firstPlayer = service.connect(connection_1);

    service.disconnect(connection_1);

    auto const secondPlayer = service.connect(connection_1);

    REQUIRE(firstPlayer == player_id{1});
    REQUIRE(secondPlayer == player_id{2});

    require_no_player(service, firstPlayer);
    require_player(service, secondPlayer);

    REQUIRE(service.state().players().size() == 1);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::apply for unknown connection is no-op",
    "[WorldService]")
{
    service.apply(connection_1, make_move_action(Position{5, 7}));
    service.apply(connection_1, make_interact_action());
    service.apply(connection_1, make_wait_action());

    REQUIRE(service.state().players().empty());

    auto const json = parse_snapshot(service);

    REQUIRE(json.at("players").empty());
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::apply MoveAction moves connected player",
    "[WorldService]")
{
    auto const player = service.connect(connection_1);
    auto const before = require_player(service, player);

    service.apply(connection_1, make_move_action(Position{3, -2}));

    auto const after = require_player(service, player);

    REQUIRE(after.position.x == before.position.x + 3);
    REQUIRE(after.position.y == before.position.y - 2);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::apply MoveAction can be applied multiple times",
    "[WorldService]")
{
    auto const player = service.connect(connection_1);
    auto const initial = require_player(service, player);

    service.apply(connection_1, make_move_action(Position{10, 5}));
    service.apply(connection_1, make_move_action(Position{-3, 7}));
    service.apply(connection_1, make_move_action(Position{0, -2}));

    auto const after = require_player(service, player);

    REQUIRE(after.position.x == initial.position.x + 7);
    REQUIRE(after.position.y == initial.position.y + 10);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::apply MoveAction affects only player bound to connection",
    "[WorldService]")
{
    auto const playerA = service.connect(connection_1);
    auto const playerB = service.connect(connection_2);

    auto const beforeA = require_player(service, playerA);
    auto const beforeB = require_player(service, playerB);

    service.apply(connection_1, make_move_action(Position{4, 9}));

    auto const afterA = require_player(service, playerA);
    auto const afterB = require_player(service, playerB);

    REQUIRE(afterA.position.x == beforeA.position.x + 4);
    REQUIRE(afterA.position.y == beforeA.position.y + 9);

    require_position(afterB.position, beforeB.position);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::apply InteractAction is no-op for connected player",
    "[WorldService]")
{
    auto const player = service.connect(connection_1);
    auto const before = require_player(service, player);

    service.apply(connection_1, make_interact_action());

    auto const after = require_player(service, player);

    REQUIRE(after.id == before.id);
    require_position(after.position, before.position);
    REQUIRE(service.state().players().size() == 1);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::apply WaitAction is no-op for connected player",
    "[WorldService]")
{
    auto const player = service.connect(connection_1);
    auto const before = require_player(service, player);

    service.apply(connection_1, make_wait_action());

    auto const after = require_player(service, player);

    REQUIRE(after.id == before.id);
    require_position(after.position, before.position);
    REQUIRE(service.state().players().size() == 1);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::snapshotJson serializes connected players and their positions",
    "[WorldService]")
{
    auto const playerA = service.connect(connection_1);
    auto const playerB = service.connect(connection_2);

    service.apply(connection_1, make_move_action(Position{5, 1}));
    service.apply(connection_2, make_move_action(Position{-2, 3}));

    auto const stateA = require_player(service, playerA);
    auto const stateB = require_player(service, playerB);

    auto const json = parse_snapshot(service);

    REQUIRE(json.at("world") == "bootstrap");
    REQUIRE(json.at("map").at("width") == 32);
    REQUIRE(json.at("map").at("height") == 18);

    auto const& players = json.at("players");

    REQUIRE(players.is_array());
    REQUIRE(players.size() == 2);

    require_json_player(players.at(0), playerA, stateA.position);
    require_json_player(players.at(1), playerB, stateB.position);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService::snapshotJson does not serialize disconnected players",
    "[WorldService]")
{
    auto const disconnectedPlayer = service.connect(connection_1);
    auto const alivePlayer = service.connect(connection_2);

    service.apply(connection_1, make_move_action(Position{10, 20}));
    service.apply(connection_2, make_move_action(Position{1, 2}));

    service.disconnect(connection_1);

    auto const aliveState = require_player(service, alivePlayer);

    REQUIRE_FALSE(service.state().player(disconnectedPlayer).has_value());

    auto const json = parse_snapshot(service);
    auto const& players = json.at("players");

    REQUIRE(players.is_array());
    REQUIRE(players.size() == 1);

    require_json_player(players.at(0), alivePlayer, aliveState.position);
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService connection binding is removed after disconnect",
    "[WorldService]")
{
    auto const oldPlayer = service.connect(connection_1);

    service.disconnect(connection_1);

    service.apply(connection_1, make_move_action(Position{100, 100}));

    REQUIRE_FALSE(service.state().player(oldPlayer).has_value());
    REQUIRE(service.state().players().empty());
}

TEST_CASE_METHOD(
    s2d::test::game::world_service_fixture,
    "WorldService can connect same connection again after disconnect",
    "[WorldService]")
{
    auto const oldPlayer = service.connect(connection_1);

    service.disconnect(connection_1);

    auto const newPlayer = service.connect(connection_1);
    auto const before = require_player(service, newPlayer);

    service.apply(connection_1, make_move_action(Position{2, 3}));

    auto const after = require_player(service, newPlayer);

    REQUIRE(newPlayer != oldPlayer);
    REQUIRE(after.position.x == before.position.x + 2);
    REQUIRE(after.position.y == before.position.y + 3);
}

} // namespace s2d::game
