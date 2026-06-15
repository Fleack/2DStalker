add_library(SharedLogger INTERFACE)

target_link_libraries(SharedLogger INTERFACE
        fmt::fmt
)

target_link_libraries(Shared INTERFACE
        SharedLogger
)