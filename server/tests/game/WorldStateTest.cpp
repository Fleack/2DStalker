#include "server/src/game/PlayerConnectionRegistry.hpp"
#include "server/src/game/WorldService.hpp"
#include "server/src/game/WorldState.hpp"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

TEST_CASE("Initial world state has a map and no players", "[game][world]")
{
    auto const state = s2d::game::WorldState::createInitial();

    REQUIRE(state.map().name == "bootstrap");
    REQUIRE(state.map().width == 32);
    REQUIRE(state.map().height == 18);
    REQUIRE(state.players().empty());
}

TEST_CASE("World state owns players and positions", "[game][world]")
{
    auto state = s2d::game::WorldState::createInitial();

    auto const playerId = state.addPlayer();

    REQUIRE(state.players().size() == 1);

    REQUIRE(state.movePlayer(playerId, s2d::game::Position{2, -1}));

    auto player = state.player(playerId);
    REQUIRE(player.has_value());
    REQUIRE(player->position == s2d::game::Position{2, -1});

    REQUIRE(state.removePlayer(playerId));

    REQUIRE(state.players().empty());
}

TEST_CASE("Player connection registry maps connections to players", "[game][world]")
{
    s2d::game::PlayerConnectionRegistry connections;

    connections.bind(10, s2d::game::player_id{1});

    REQUIRE(connections.playerFor(10) == s2d::game::player_id{1});

    connections.unbind(10);

    REQUIRE_FALSE(connections.playerFor(10).has_value());
}

TEST_CASE("World snapshot is projected from world state", "[game][world]")
{
    auto state = s2d::game::WorldState::createInitial();
    auto const playerId = state.addPlayer();
    state.movePlayer(playerId, s2d::game::Position{3, 4});

    auto const snapshot = nlohmann::json::parse(state.toJson());

    REQUIRE(snapshot.at("world") == "bootstrap");
    REQUIRE(snapshot.at("map").at("width") == 32);
    REQUIRE(snapshot.at("map").at("height") == 18);

    auto const& players = snapshot.at("players");
    REQUIRE(players.size() == 1);
    REQUIRE(players.at(0).at("id") == playerId);
    REQUIRE(players.at(0).at("position").at("x") == 3);
    REQUIRE(players.at(0).at("position").at("y") == 4);
}

TEST_CASE("World service coordinates connections, state and actions", "[game][world]")
{
    s2d::game::WorldService service;

    auto const playerId = service.connect(7);
    auto const samePlayerId = service.connect(7);

    REQUIRE(samePlayerId == playerId);

    service.apply(7, s2d::game::MoveAction{{1, 2}});
    service.apply(7, s2d::game::InteractAction{});
    service.apply(7, s2d::game::WaitAction{});

    auto player = service.state().player(playerId);
    REQUIRE(player.has_value());
    REQUIRE(player->position == s2d::game::Position{1, 2});

    service.disconnect(7);

    REQUIRE_FALSE(service.state().player(playerId).has_value());
}
