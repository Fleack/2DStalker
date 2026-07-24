s2d_add_test(
        client-network-test
        SOURCES
        ClientTest.cpp
        LIBS
        ClientNetwork)

s2d_add_test(
        client-connection-manager-test
        SOURCES
        ConnectionManagerTest.cpp
        LIBS
        ClientNetwork)

s2d_add_test(
        client-request-manager-test
        SOURCES
        ClientRequestManagerTest.cpp
        LIBS
        ClientNetwork)
