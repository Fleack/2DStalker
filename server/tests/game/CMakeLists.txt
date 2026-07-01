s2d_add_test(server-game-world-state-test
        SOURCES
        WorldStateTest.cpp
        LIBS
        ServerGame
        json_headers)

s2d_add_test(server-game-player-connection-registry-test
        SOURCES
        PlayerConnectionRegistryTest.cpp
        LIBS
        ServerGame)

s2d_add_test(server-game-world-service-test
        SOURCES
        WorldServiceTest.cpp
        LIBS
        ServerGame
        json_headers)

add_custom_target(server-game-tests)
add_dependencies(server-game-tests
        server-game-world-state-test
        server-game-player-connection-registry-test
        server-game-world-service-test
)