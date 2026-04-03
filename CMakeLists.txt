cmake_minimum_required(VERSION 3.24)
project(StalkerGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(_WIN32_WINNT=0x0601)

include(FetchContent)

# JSON
FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.12.0
)

# spdlog
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.17.0
)

# SFML
FetchContent_Declare(
        sfml
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
)

# SQLiteCpp
FetchContent_Declare(
        sqlitecpp
        GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
        GIT_TAG 3.3.3
)

# Boost Asio
FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
)

FetchContent_MakeAvailable(json spdlog sfml sqlitecpp asio)

# ====== Shared ======
add_library(Shared INTERFACE)
target_include_directories(Shared INTERFACE shared/include)
target_link_libraries(Shared INTERFACE nlohmann_json::nlohmann_json)

# ====== Server ======
add_executable(StalkerServer
        server/src/main.cpp
)
target_include_directories(StalkerServer PRIVATE
        ${asio_SOURCE_DIR}/asio/include
        ${sqlitecpp_SOURCE_DIR}/include
)
target_link_libraries(StalkerServer PRIVATE
        Shared
        spdlog::spdlog
        SQLiteCpp
        ws2_32
        mswsock
)

# ====== Client ======
add_executable(StalkerClient
        client/src/main.cpp
)
target_include_directories(StalkerClient PRIVATE
        ${asio_SOURCE_DIR}/asio/include
        ${sqlitecpp_SOURCE_DIR}/include
)
target_link_libraries(StalkerClient PRIVATE
        Shared
        spdlog::spdlog
        sfml-graphics
        sfml-window
        sfml-system
        sfml-audio
        ws2_32
        mswsock
)
