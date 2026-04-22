add_subdirectory(src/network)

add_executable(StalkerServer
        src/main.cpp
)

target_link_libraries(StalkerServer PRIVATE
        Shared
        ServerNetwork
)
