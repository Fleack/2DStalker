add_subdirectory(game)
add_subdirectory(network)

add_executable(Server
        main.cpp
)

target_link_libraries(Server PRIVATE
        ServerGame
        ServerNetwork
)

copy_runtime_dlls(Server)

enable_gcc_flags(Server)
