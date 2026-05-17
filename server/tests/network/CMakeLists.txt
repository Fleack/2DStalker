add_executable(ServerNetworkTests
        ConnectionWriteQueueTests.cpp
)

target_link_libraries(ServerNetworkTests PRIVATE
        Catch2::Catch2WithMain
        ServerNetwork
        Shared
)

if (WIN32)
    target_link_libraries(ServerNetworkTests PRIVATE
            ws2_32
            mswsock
    )
endif ()

add_test(NAME ServerNetworkTests COMMAND ServerNetworkTests)
