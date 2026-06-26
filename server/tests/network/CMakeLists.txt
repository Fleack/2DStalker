s2d_add_test(server-connection-manager-test
        SOURCES
        ServerConnectionManagerTest.cpp
        LIBS
        ServerNetwork
        ServerGame
        SharedNetwork
        SharedProtocol)
