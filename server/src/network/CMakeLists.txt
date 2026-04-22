add_library(ServerNetwork
        Server.cpp
        Session.cpp
        Session.hpp)

target_link_libraries(ServerNetwork PRIVATE
        Shared
        ws2_32
        mswsock
)
