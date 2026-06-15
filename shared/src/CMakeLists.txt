add_library(Shared INTERFACE)

target_include_directories(Shared INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(shared/protocol)
add_subdirectory(shared/network)
add_subdirectory(shared/logger)