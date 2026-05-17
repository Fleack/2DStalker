add_library(ServerNetwork
        Connection.cpp
        ConnectionWriter.cpp
        ConnectionManager.cpp
        ServerMessageHandler.cpp
        TcpServer.cpp)

target_link_libraries(ServerNetwork PUBLIC Shared)

target_include_directories(ServerNetwork PUBLIC
        ${PROJECT_SOURCE_DIR}
)

if (WIN32)
    target_link_libraries(ServerNetwork PRIVATE
            ws2_32
            mswsock
    )
endif ()
