add_library(Shared INTERFACE)

target_include_directories(Shared INTERFACE
        src
)

add_subdirectory(src/shared/protocol)

target_link_libraries(Shared INTERFACE
        asio_headers
        json_headers
        StalkerProtocol
        spdlog::spdlog
        fmt::fmt
)