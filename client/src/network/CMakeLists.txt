add_library(ClientNetwork STATIC
        Client.cpp
)

target_include_directories(ClientNetwork PUBLIC
        ${PROJECT_SOURCE_DIR}/client/src
)

target_link_libraries(ClientNetwork PUBLIC
        SharedNetwork
        SharedProtocol
        SharedLogger
)

enable_gcc_flags(ClientNetwork)
