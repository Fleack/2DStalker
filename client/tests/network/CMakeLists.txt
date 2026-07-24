s2d_add_test(
        client-network-test
        SOURCES
        ClientTest.cpp
        LIBS
        ClientNetwork
        SharedNetwork
        SharedProtocol)

s2d_add_test(
        client-connection-manager-test
        SOURCES
        ConnectionManagerTest.cpp
        LIBS
        ClientNetwork)

