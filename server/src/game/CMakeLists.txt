add_library(ServerGame
        PlayerConnectionRegistry.cpp
        WorldService.cpp
        WorldState.cpp)

target_include_directories(ServerGame PUBLIC
        ${PROJECT_SOURCE_DIR}
)

target_link_libraries(ServerGame PRIVATE
        json_headers
        SharedLogger)

enable_gcc_flags(ServerGame)
