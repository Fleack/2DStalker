add_library(SharedNetwork INTERFACE)

target_link_libraries(SharedNetwork INTERFACE
        Boost::asio
        SharedLogger
)

if (WIN32)
    target_compile_definitions(SharedNetwork INTERFACE
            _WIN32_WINNT=0x0A00
    )
endif ()

if (WIN32)
    target_link_libraries(SharedNetwork INTERFACE
            ws2_32
            mswsock
    )
else ()
    message(FATAL_ERROR "Not windows build is not supported!")
endif ()

target_include_directories(SharedNetwork INTERFACE ${PROJECT_SOURCE_DIR}/shared/src)
