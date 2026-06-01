target_sources(Client PRIVATE
        Client.cpp
)

target_link_libraries(Client PRIVATE asio_headers)

if (WIN32)
    target_link_libraries(Client PRIVATE
            ws2_32
            mswsock
    )
endif ()
