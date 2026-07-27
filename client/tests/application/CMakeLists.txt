s2d_add_test(
        client-state-manager-test
        SOURCES
        StateManagerTest.cpp
        LIBS
        ClientApplication)

s2d_add_test(
        client-network-component-test
        SOURCES
        NetworkComponentTest.cpp
        LIBS
        ClientNetwork)
