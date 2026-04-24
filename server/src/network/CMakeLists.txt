add_library(ServerNetwork
        Connection.cpp
        ConnectionManager.cpp
        MessageChannel.cpp
        ServerMessageHandler.cpp
        TcpServer.cpp)

target_link_libraries(ServerNetwork PRIVATE Shared)

if (WIN32)
    target_link_libraries(ServerNetwork PRIVATE
            ws2_32
            mswsock
    )
endif()
