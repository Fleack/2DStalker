add_library(SharedNetwork INTERFACE)

target_link_libraries(SharedNetwork INTERFACE
        asio_headers
        SharedLogger
)

if (WIN32)
    target_link_libraries(SharedNetwork INTERFACE
            ws2_32
            mswsock
    )
else ()
    message(FATAL_ERROR "Not windows build is not supported!")
endif ()

target_include_directories(SharedNetwork INTERFACE ${PROJECT_SOURCE_DIR}/shared/src)