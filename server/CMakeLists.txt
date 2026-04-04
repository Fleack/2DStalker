add_executable(StalkerServer
        src/main.cpp
)
target_include_directories(StalkerServer PRIVATE
        ${asio_SOURCE_DIR}/asio/include
        ${sqlitecpp_SOURCE_DIR}/include
)
target_link_libraries(StalkerServer PRIVATE
        Shared
        SQLiteCpp
        ws2_32
        mswsock
)