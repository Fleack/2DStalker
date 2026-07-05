add_library(ClientNetwork
        Client.cpp
)

target_include_directories(ClientNetwork PUBLIC
        ${PROJECT_SOURCE_DIR}/client/src
        ${PROJECT_SOURCE_DIR}/shared/src
)

target_link_libraries(ClientNetwork
        PUBLIC
        SharedNetwork
        SharedProtocol
        PRIVATE
        SharedLogger
)

enable_gcc_flags(ClientNetwork)