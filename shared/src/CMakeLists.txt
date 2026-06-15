add_library(Shared INTERFACE)

target_include_directories(Shared INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(shared/protocol)

target_link_libraries(Shared INTERFACE
        asio_headers
        json_headers
        SharedProtocol
        spdlog::spdlog
        fmt::fmt
)

if (WIN32)
    target_link_libraries(Shared INTERFACE
            ws2_32
            mswsock
    )
endif ()