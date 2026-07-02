FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.1.0
)
FetchContent_MakeAvailable(sfml)

add_executable(Client
        main.cpp
)

add_subdirectory(network)

target_link_libraries(Client PRIVATE
        ClientNetwork
        SharedLogger
        SharedProtocol
        sfml-graphics
        sfml-window
        sfml-system
        json_headers
)

copy_runtime_dlls(Client)

enable_gcc_flags(Client)
