add_subdirectory(network)

add_executable(Server
        main.cpp
)

target_link_libraries(Server PRIVATE
        SharedProtocol
        ServerNetwork
)
enable_gcc_flags(Server)
