#include "server/src/game/WorldService.hpp"

#include <optional>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

namespace s2d::game
{
namespace
{

constexpr connection_key connection1{1};
constexpr connection_key connection2{2};
constexpr connection_key connection3{3};

nlohmann::json parseSnapshot(WorldService const& service)
{
    return nlohmann::json::parse(service.snapshotJson());
}

void requireBootstrapMap(WorldService const& service)
{
    REQUIRE(service.state().map().name == "bootstrap");
    REQUIRE(service.state().map().width == 32);
    REQUIRE(service.state().map().height == 18);
}

void requireNoPlayer(WorldService const& service, player_id player)
{
    REQUIRE_FALSE(service.state().player(player).has_value());
}

PlayerState requirePlayer(WorldService const& service, player_id player)
{
    auto const playerState = service.state().player(player);

    REQUIRE(playerState.has_value());
    REQUIRE(playerState->id == player);

    return playerState.value();
}

void requirePosition(Position const& actual, Position const& expected)
{
    REQUIRE(actual.x == expected.x);
    REQUIRE(actual.y == expected.y);
}

void requireJsonPlayer(
    nlohmann::json const& json,
    player_id expectedId,
    Position const& expectedPosition)
{
    REQUIRE(json.at("id").get<player_id>() == expectedId);
    REQUIRE(json.at("position").at("x").get<decltype(expectedPosition.x)>() == expectedPosition.x);
    REQUIRE(json.at("position").at("y").get<decltype(expectedPosition.y)>() == expectedPosition.y);
}

WorldAction moveAction(Position delta)
{
    return WorldAction{MoveAction{delta}};
}

WorldAction interactAction()
{
    return WorldAction{InteractAction{}};
}

WorldAction waitAction()
{
    return WorldAction{WaitAction{}};
}

} // namespace

TEST_CASE("WorldService default constructor creates initial empty world", "[WorldService]")
{
    WorldService service;

    requireBootstrapMap(service);

    REQUIRE(service.state().players().empty());

    auto const json = parseSnapshot(service);

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

    auto const json = parseSnapshot(service);

    REQUIRE(json.at("world") == "custom-map");
    REQUIRE(json.at("map").at("width") == 100);
    REQUIRE(json.at("map").at("height") == 50);
    REQUIRE(json.at("players").empty());
}

TEST_CASE("WorldService::connect creates player for new connection", "[WorldService]")
{
    WorldService service;

    auto const player = service.connect(connection1);

    REQUIRE(player == player_id{1});
    REQUIRE(service.state().players().size() == 1);

    auto const playerState = requirePlayer(service, player);

    REQUIRE(playerState.id == player);
}

TEST_CASE("WorldService::connect returns existing player for already connected connection", "[WorldService]")
{
    WorldService service;

    auto const firstConnect = service.connect(connection1);
    auto const secondConnect = service.connect(connection1);

    REQUIRE(secondConnect == firstConnect);
    REQUIRE(service.state().players().size() == 1);

    requirePlayer(service, firstConnect);
}

TEST_CASE("WorldService::connect creates independent players for different connections", "[WorldService]")
{
    WorldService service;

    auto const playerA = service.connect(connection1);
    auto const playerB = service.connect(connection2);
    auto const playerC = service.connect(connection3);

    REQUIRE(playerA == player_id{1});
    REQUIRE(playerB == player_id{2});
    REQUIRE(playerC == player_id{3});

    REQUIRE(service.state().players().size() == 3);

    requirePlayer(service, playerA);
    requirePlayer(service, playerB);
    requirePlayer(service, playerC);
}

TEST_CASE("WorldService::disconnect unknown connection is no-op", "[WorldService]")
{
    WorldService service;

    service.disconnect(connection1);

    REQUIRE(service.state().players().empty());

    auto const json = parseSnapshot(service);

    REQUIRE(json.at("players").empty());
}

TEST_CASE("WorldService::disconnect removes connected player", "[WorldService]")
{
    WorldService service;

    auto const player = service.connect(connection1);

    REQUIRE(service.state().players().size() == 1);
    requirePlayer(service, player);

    service.disconnect(connection1);

    REQUIRE(service.state().players().empty());
    requireNoPlayer(service, player);
}

TEST_CASE("WorldService::disconnect removes only player for requested connection", "[WorldService]")
{
    WorldService service;

    auto const playerA = service.connect(connection1);
    auto const playerB = service.connect(connection2);

    service.disconnect(connection1);

    REQUIRE(service.state().players().size() == 1);

    requireNoPlayer(service, playerA);
    requirePlayer(service, playerB);
}

TEST_CASE("WorldService::disconnect can be called repeatedly for same connection", "[WorldService]")
{
    WorldService service;

    auto const player = service.connect(connection1);

    service.disconnect(connection1);
    service.disconnect(connection1);
    service.disconnect(connection1);

    REQUIRE(service.state().players().empty());
    requireNoPlayer(service, player);
}

TEST_CASE("WorldService reconnect after disconnect creates new player id", "[WorldService]")
{
    WorldService service;

    auto const firstPlayer = service.connect(connection1);

    service.disconnect(connection1);

    auto const secondPlayer = service.connect(connection1);

    REQUIRE(firstPlayer == player_id{1});
    REQUIRE(secondPlayer == player_id{2});

    requireNoPlayer(service, firstPlayer);
    requirePlayer(service, secondPlayer);

    REQUIRE(service.state().players().size() == 1);
}

TEST_CASE("WorldService::apply for unknown connection is no-op", "[WorldService]")
{
    WorldService service;

    service.apply(connection1, moveAction(Position{5, 7}));
    service.apply(connection1, interactAction());
    service.apply(connection1, waitAction());

    REQUIRE(service.state().players().empty());

    auto const json = parseSnapshot(service);

    REQUIRE(json.at("players").empty());
}

TEST_CASE("WorldService::apply MoveAction moves connected player", "[WorldService]")
{
    WorldService service;

    auto const player = service.connect(connection1);
    auto const before = requirePlayer(service, player);

    service.apply(connection1, moveAction(Position{3, -2}));

    auto const after = requirePlayer(service, player);

    REQUIRE(after.position.x == before.position.x + 3);
    REQUIRE(after.position.y == before.position.y - 2);
}

TEST_CASE("WorldService::apply MoveAction can be applied multiple times", "[WorldService]")
{
    WorldService service;

    auto const player = service.connect(connection1);
    auto const initial = requirePlayer(service, player);

    service.apply(connection1, moveAction(Position{10, 5}));
    service.apply(connection1, moveAction(Position{-3, 7}));
    service.apply(connection1, moveAction(Position{0, -2}));

    auto const after = requirePlayer(service, player);

    REQUIRE(after.position.x == initial.position.x + 7);
    REQUIRE(after.position.y == initial.position.y + 10);
}

TEST_CASE("WorldService::apply MoveAction affects only player bound to connection", "[WorldService]")
{
    WorldService service;

    auto const playerA = service.connect(connection1);
    auto const playerB = service.connect(connection2);

    auto const beforeA = requirePlayer(service, playerA);
    auto const beforeB = requirePlayer(service, playerB);

    service.apply(connection1, moveAction(Position{4, 9}));

    auto const afterA = requirePlayer(service, playerA);
    auto const afterB = requirePlayer(service, playerB);

    REQUIRE(afterA.position.x == beforeA.position.x + 4);
    REQUIRE(afterA.position.y == beforeA.position.y + 9);

    requirePosition(afterB.position, beforeB.position);
}

TEST_CASE("WorldService::apply InteractAction is no-op for connected player", "[WorldService]")
{
    WorldService service;

    auto const player = service.connect(connection1);
    auto const before = requirePlayer(service, player);

    service.apply(connection1, interactAction());

    auto const after = requirePlayer(service, player);

    REQUIRE(after.id == before.id);
    requirePosition(after.position, before.position);
    REQUIRE(service.state().players().size() == 1);
}

TEST_CASE("WorldService::apply WaitAction is no-op for connected player", "[WorldService]")
{
    WorldService service;

    auto const player = service.connect(connection1);
    auto const before = requirePlayer(service, player);

    service.apply(connection1, waitAction());

    auto const after = requirePlayer(service, player);

    REQUIRE(after.id == before.id);
    requirePosition(after.position, before.position);
    REQUIRE(service.state().players().size() == 1);
}

TEST_CASE("WorldService::snapshotJson serializes connected players and their positions", "[WorldService]")
{
    WorldService service;

    auto const playerA = service.connect(connection1);
    auto const playerB = service.connect(connection2);

    service.apply(connection1, moveAction(Position{5, 1}));
    service.apply(connection2, moveAction(Position{-2, 3}));

    auto const stateA = requirePlayer(service, playerA);
    auto const stateB = requirePlayer(service, playerB);

    auto const json = parseSnapshot(service);

    REQUIRE(json.at("world") == "bootstrap");
    REQUIRE(json.at("map").at("width") == 32);
    REQUIRE(json.at("map").at("height") == 18);

    auto const& players = json.at("players");

    REQUIRE(players.is_array());
    REQUIRE(players.size() == 2);

    requireJsonPlayer(players.at(0), playerA, stateA.position);
    requireJsonPlayer(players.at(1), playerB, stateB.position);
}

TEST_CASE("WorldService::snapshotJson does not serialize disconnected players", "[WorldService]")
{
    WorldService service;

    auto const disconnectedPlayer = service.connect(connection1);
    auto const alivePlayer = service.connect(connection2);

    service.apply(connection1, moveAction(Position{10, 20}));
    service.apply(connection2, moveAction(Position{1, 2}));

    service.disconnect(connection1);

    auto const aliveState = requirePlayer(service, alivePlayer);

    REQUIRE_FALSE(service.state().player(disconnectedPlayer).has_value());

    auto const json = parseSnapshot(service);
    auto const& players = json.at("players");

    REQUIRE(players.is_array());
    REQUIRE(players.size() == 1);

    requireJsonPlayer(players.at(0), alivePlayer, aliveState.position);
}

TEST_CASE("WorldService connection binding is removed after disconnect", "[WorldService]")
{
    WorldService service;

    auto const oldPlayer = service.connect(connection1);

    service.disconnect(connection1);

    service.apply(connection1, moveAction(Position{100, 100}));

    REQUIRE_FALSE(service.state().player(oldPlayer).has_value());
    REQUIRE(service.state().players().empty());
}

TEST_CASE("WorldService can connect same connection again after disconnect", "[WorldService]")
{
    WorldService service;

    auto const oldPlayer = service.connect(connection1);

    service.disconnect(connection1);

    auto const newPlayer = service.connect(connection1);
    auto const before = requirePlayer(service, newPlayer);

    service.apply(connection1, moveAction(Position{2, 3}));

    auto const after = requirePlayer(service, newPlayer);

    REQUIRE(newPlayer != oldPlayer);
    REQUIRE(after.position.x == before.position.x + 2);
    REQUIRE(after.position.y == before.position.y + 3);
}

} // namespace s2d::game
