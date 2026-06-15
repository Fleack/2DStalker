add_library(SharedNetwork INTERFACE)

target_link_libraries(SharedNetwork INTERFACE
        asio_headers
        spdlog::spdlog
)

if (WIN32)
    target_link_libraries(Shared INTERFACE
            ws2_32
            mswsock
    )
endif ()

target_link_libraries(Shared INTERFACE
        SharedNetwork
)