FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
)
FetchContent_MakeAvailable(sfml)

add_executable(Client
        main.cpp
)

add_subdirectory(network)

target_link_libraries(Client PRIVATE
        SharedNetwork
        SharedLogger
        SharedProtocol
        sfml-graphics
        sfml-window
        sfml-system
        json_headers
)
enable_gcc_flags(Client)
