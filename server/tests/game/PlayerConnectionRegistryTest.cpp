#include "server/src/game/PlayerConnectionRegistry.hpp"

#include <optional>

#include <catch2/catch_test_macros.hpp>

namespace s2d::game
{
namespace
{

constexpr connection_key connection1{1};
constexpr connection_key connection2{2};
constexpr connection_key connection3{3};

constexpr player_id player1{10};
constexpr player_id player2{20};
constexpr player_id player3{30};

} // namespace

TEST_CASE("PlayerConnectionRegistry::playerFor returns nullopt for unknown connection", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    REQUIRE_FALSE(registry.playerFor(connection1).has_value());
    REQUIRE_FALSE(registry.playerFor(connection2).has_value());
    REQUIRE_FALSE(registry.playerFor(connection3).has_value());
}

TEST_CASE("PlayerConnectionRegistry::bind binds connection to player", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);

    auto const player = registry.playerFor(connection1);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player1);
}

TEST_CASE("PlayerConnectionRegistry::bind supports multiple independent connections", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);
    registry.bind(connection2, player2);
    registry.bind(connection3, player3);

    REQUIRE(registry.playerFor(connection1).has_value());
    REQUIRE(registry.playerFor(connection2).has_value());
    REQUIRE(registry.playerFor(connection3).has_value());

    REQUIRE(registry.playerFor(connection1).value() == player1);
    REQUIRE(registry.playerFor(connection2).value() == player2);
    REQUIRE(registry.playerFor(connection3).value() == player3);
}

TEST_CASE("PlayerConnectionRegistry::bind overwrites existing binding for same connection", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);

    REQUIRE(registry.playerFor(connection1).has_value());
    REQUIRE(registry.playerFor(connection1).value() == player1);

    registry.bind(connection1, player2);

    auto const player = registry.playerFor(connection1);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player2);
}

TEST_CASE("PlayerConnectionRegistry::bind overwrite does not affect other connections", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);
    registry.bind(connection2, player2);

    registry.bind(connection1, player3);

    REQUIRE(registry.playerFor(connection1).has_value());
    REQUIRE(registry.playerFor(connection2).has_value());

    REQUIRE(registry.playerFor(connection1).value() == player3);
    REQUIRE(registry.playerFor(connection2).value() == player2);
}

TEST_CASE("PlayerConnectionRegistry::unbind removes existing binding", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);

    REQUIRE(registry.playerFor(connection1).has_value());

    registry.unbind(connection1);

    REQUIRE_FALSE(registry.playerFor(connection1).has_value());
}

TEST_CASE("PlayerConnectionRegistry::unbind unknown connection is no-op", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.unbind(connection1);

    REQUIRE_FALSE(registry.playerFor(connection1).has_value());
}

TEST_CASE("PlayerConnectionRegistry::unbind unknown connection does not affect existing bindings", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);
    registry.bind(connection2, player2);

    registry.unbind(connection3);

    REQUIRE(registry.playerFor(connection1).has_value());
    REQUIRE(registry.playerFor(connection2).has_value());
    REQUIRE_FALSE(registry.playerFor(connection3).has_value());

    REQUIRE(registry.playerFor(connection1).value() == player1);
    REQUIRE(registry.playerFor(connection2).value() == player2);
}

TEST_CASE("PlayerConnectionRegistry::unbind removes only requested connection", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);
    registry.bind(connection2, player2);

    registry.unbind(connection1);

    REQUIRE_FALSE(registry.playerFor(connection1).has_value());

    auto const player = registry.playerFor(connection2);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player2);
}

TEST_CASE("PlayerConnectionRegistry::unbind can be called repeatedly", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);

    registry.unbind(connection1);
    registry.unbind(connection1);
    registry.unbind(connection1);

    REQUIRE_FALSE(registry.playerFor(connection1).has_value());
}

TEST_CASE("PlayerConnectionRegistry can bind connection again after unbind", "[PlayerConnectionRegistry]")
{
    PlayerConnectionRegistry registry;

    registry.bind(connection1, player1);
    registry.unbind(connection1);

    REQUIRE_FALSE(registry.playerFor(connection1).has_value());

    registry.bind(connection1, player2);

    auto const player = registry.playerFor(connection1);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player2);
}

} // namespace s2d::game
