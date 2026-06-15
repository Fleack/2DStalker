add_library(SharedLogger INTERFACE)

target_link_libraries(SharedLogger INTERFACE
        fmt::fmt
        spdlog::spdlog
)

target_include_directories(SharedLogger INTERFACE ${PROJECT_SOURCE_DIR}/shared/src)