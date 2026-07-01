s2d_add_test(shared-network-connection-send-queue-test
        SOURCES
        ConnectionSendQueueTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)

s2d_add_test(shared-network-message-channel-test
        SOURCES
        MessageChannelTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)

s2d_add_test(shared-network-message-frame-codec-test
        SOURCES
        MessageFrameCodecTest.cpp
        LIBS
        SharedNetwork)

s2d_add_test(shared-network-connection-test
        SOURCES
        ConnectionTest.cpp
        LIBS
        SharedNetwork
        SharedProtocol)

add_custom_target(shared-network-tests)
add_dependencies(shared-network-tests
        shared-network-connection-send-queue-test
        shared-network-message-channel-test
        shared-network-message-frame-codec-test
        shared-network-connection-test
)
