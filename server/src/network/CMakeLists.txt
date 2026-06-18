add_library(ServerNetwork
        ServerConnectionManager.cpp
        ServerMessageHandler.cpp
        TcpServer.cpp)

target_include_directories(ServerNetwork PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ServerNetwork PUBLIC
        ServerGame
        SharedNetwork
        SharedProtocol
        SharedLogger)

enable_gcc_flags(ServerNetwork)
