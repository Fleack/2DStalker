add_library(Shared INTERFACE)

target_include_directories(Shared INTERFACE
        src
        ${asio_SOURCE_DIR}/asio/include
)

add_subdirectory(src/protocol)

target_link_libraries(Shared INTERFACE
        StalkerProtocol
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        fmt::fmt
)