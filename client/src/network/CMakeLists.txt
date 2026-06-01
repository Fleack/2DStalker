target_sources(StalkerClient PRIVATE
        Client.cpp
)

target_link_libraries(StalkerClient PRIVATE asio_headers)
