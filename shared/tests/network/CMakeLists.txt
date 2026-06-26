s2d_add_test(
        connection-writer-test
        SOURCES
        ConnectionWriterTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)

s2d_add_test(message-channel-test
        SOURCES
        MessageChannelTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)

s2d_add_test(connection-test
        SOURCES
        ConnectionTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)
