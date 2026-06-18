s2d_add_test(world-state-test
        SOURCES
        WorldStateTest.cpp
        LIBS
        ServerGame
        json_headers)

s2d_add_test(player-connection-registry-test
        SOURCES
        PlayerConnectionRegistryTest.cpp
        LIBS
        ServerGame)

s2d_add_test(world-service-test
        SOURCES
        WorldServiceTest.cpp
        LIBS
        ServerGame
        json_headers)