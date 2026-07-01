s2d_add_test(connection-send-queue-test
        SOURCES
        ConnectionSendQueueTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)

s2d_add_test(message-channel-test
        SOURCES
        MessageChannelTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)

s2d_add_test(message-frame-codec-test
        SOURCES
        MessageFrameCodecTest.cpp
        LIBS
        SharedNetwork)

s2d_add_test(connection-test
        SOURCES
        ConnectionTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)
