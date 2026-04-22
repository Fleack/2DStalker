add_executable(StalkerClient
        src/main.cpp
)

add_subdirectory(src/network)

target_link_libraries(StalkerClient PRIVATE
        Shared
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        ws2_32
        mswsock
)