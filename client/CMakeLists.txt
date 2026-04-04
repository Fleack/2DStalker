add_executable(StalkerClient
        src/main.cpp
)

add_subdirectory(src/network)

target_include_directories(StalkerClient PRIVATE
        ${sqlitecpp_SOURCE_DIR}/include
)
target_link_libraries(StalkerClient PRIVATE
        Shared
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        ws2_32
        mswsock
)