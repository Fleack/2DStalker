#pragma once

#include "server/src/game/PlayerConnectionRegistry.hpp"
#include "server/src/game/WorldService.hpp"
#include "server/src/game/WorldState.hpp"

namespace s2d::test::game
{

struct player_connection_registry_fixture
{
    s2d::game::PlayerConnectionRegistry registry;
};

struct world_state_fixture
{
    s2d::game::WorldState world{s2d::game::WorldState::createInitial()};
};

struct world_service_fixture
{
    s2d::game::WorldService service;
};

} // namespace s2d::test::game
