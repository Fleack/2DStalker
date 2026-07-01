s2d_add_test(server-network-connection-manager-test
        SOURCES
        ServerConnectionManagerTest.cpp
        LIBS
        ServerNetwork
        ServerGame
        SharedNetwork
        SharedProtocol)

add_custom_target(server-network-tests)
add_dependencies(server-network-tests
        server-network-connection-manager-test
)
