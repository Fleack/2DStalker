#include "server/src/game/PlayerConnectionRegistry.hpp"
#include "utils/game_fixtures.hpp"
#include "utils/game_test_constants.hpp"

#include <catch2/catch_test_macros.hpp>

namespace s2d::game
{
using s2d::test::game::connection_1;
using s2d::test::game::connection_2;
using s2d::test::game::connection_3;
using s2d::test::game::player_1;
using s2d::test::game::player_2;
using s2d::test::game::player_3;

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::playerFor returns nullopt for unknown connection",
    "[PlayerConnectionRegistry]")
{
    REQUIRE_FALSE(registry.playerFor(connection_1).has_value());
    REQUIRE_FALSE(registry.playerFor(connection_2).has_value());
    REQUIRE_FALSE(registry.playerFor(connection_3).has_value());
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::bind binds connection to player",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);

    auto const player = registry.playerFor(connection_1);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player_1);
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::bind supports multiple independent connections",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);
    registry.bind(connection_2, player_2);
    registry.bind(connection_3, player_3);

    REQUIRE(registry.playerFor(connection_1).has_value());
    REQUIRE(registry.playerFor(connection_2).has_value());
    REQUIRE(registry.playerFor(connection_3).has_value());

    REQUIRE(registry.playerFor(connection_1).value() == player_1);
    REQUIRE(registry.playerFor(connection_2).value() == player_2);
    REQUIRE(registry.playerFor(connection_3).value() == player_3);
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::bind overwrites existing binding for same connection",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);

    REQUIRE(registry.playerFor(connection_1).has_value());
    REQUIRE(registry.playerFor(connection_1).value() == player_1);

    registry.bind(connection_1, player_2);

    auto const player = registry.playerFor(connection_1);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player_2);
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::bind overwrite does not affect other connections",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);
    registry.bind(connection_2, player_2);

    registry.bind(connection_1, player_3);

    REQUIRE(registry.playerFor(connection_1).has_value());
    REQUIRE(registry.playerFor(connection_2).has_value());

    REQUIRE(registry.playerFor(connection_1).value() == player_3);
    REQUIRE(registry.playerFor(connection_2).value() == player_2);
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::unbind removes existing binding",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);

    REQUIRE(registry.playerFor(connection_1).has_value());

    registry.unbind(connection_1);

    REQUIRE_FALSE(registry.playerFor(connection_1).has_value());
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::unbind unknown connection is no-op",
    "[PlayerConnectionRegistry]")
{
    registry.unbind(connection_1);

    REQUIRE_FALSE(registry.playerFor(connection_1).has_value());
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::unbind unknown connection does not affect existing bindings",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);
    registry.bind(connection_2, player_2);

    registry.unbind(connection_3);

    REQUIRE(registry.playerFor(connection_1).has_value());
    REQUIRE(registry.playerFor(connection_2).has_value());
    REQUIRE_FALSE(registry.playerFor(connection_3).has_value());

    REQUIRE(registry.playerFor(connection_1).value() == player_1);
    REQUIRE(registry.playerFor(connection_2).value() == player_2);
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::unbind removes only requested connection",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);
    registry.bind(connection_2, player_2);

    registry.unbind(connection_1);

    REQUIRE_FALSE(registry.playerFor(connection_1).has_value());

    auto const player = registry.playerFor(connection_2);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player_2);
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry::unbind can be called repeatedly",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);

    registry.unbind(connection_1);
    registry.unbind(connection_1);
    registry.unbind(connection_1);

    REQUIRE_FALSE(registry.playerFor(connection_1).has_value());
}

TEST_CASE_METHOD(
    s2d::test::game::player_connection_registry_fixture,
    "PlayerConnectionRegistry can bind connection again after unbind",
    "[PlayerConnectionRegistry]")
{
    registry.bind(connection_1, player_1);
    registry.unbind(connection_1);

    REQUIRE_FALSE(registry.playerFor(connection_1).has_value());

    registry.bind(connection_1, player_2);

    auto const player = registry.playerFor(connection_1);

    REQUIRE(player.has_value());
    REQUIRE(player.value() == player_2);
}

} // namespace s2d::game
