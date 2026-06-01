cmake_minimum_required(VERSION 3.24)
project(StalkerGame LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(enable_gcc_flags target)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        return()
    endif ()

    target_compile_options(${target} PRIVATE
            -Werror
            -Wall
            -Wextra
            -Wpedantic

            -Wformat=2
            -Wconversion
            -Wsign-conversion
            -Wshadow=compatible-local
            -Wold-style-cast
            -Wcast-align=strict
            -Wcast-qual
            -Wzero-as-null-pointer-constant
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wsuggest-override
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wnull-dereference
            -Wdouble-promotion
            -Wswitch-enum
            -Wundef
            -Wuseless-cast
            -Wvla
            -Walloca
            -Wmissing-declarations

            -fdiagnostics-color=always
            -fdiagnostics-show-option

            # Debug config
            $<$<CONFIG:Debug>:-Og>
            $<$<CONFIG:Debug>:-g3>
            $<$<CONFIG:Debug>:-ggdb>
            $<$<CONFIG:Debug>:-fno-omit-frame-pointer>

            # RelWithDebInfo config
            $<$<CONFIG:RelWithDebInfo>:-O2>
            $<$<CONFIG:RelWithDebInfo>:-g>
            $<$<CONFIG:RelWithDebInfo>:-fno-omit-frame-pointer>

            # Release config
            $<$<CONFIG:Release>:-O2>
    )
endfunction()

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

# Boost Asio
FetchContent_Declare(
        asio
        GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
        GIT_TAG asio-1-30-2
)

# Fmt
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.1.0
)

# Catch2
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.13.0
)

FetchContent_MakeAvailable(json spdlog sfml asio fmt Catch2)

find_package(Protobuf CONFIG REQUIRED)

# ====== Json headers ======
add_library(json_headers INTERFACE)

target_include_directories(json_headers SYSTEM INTERFACE
        ${json_SOURCE_DIR}/include
)

# ====== Asio headers ======
add_library(asio_headers INTERFACE)

target_include_directories(asio_headers SYSTEM INTERFACE
        ${asio_SOURCE_DIR}/asio/include
)

target_compile_definitions(asio_headers INTERFACE
        ASIO_STANDALONE
)

if (WIN32)
    target_compile_definitions(asio_headers INTERFACE
            _WIN32_WINNT=0x0601
    )
endif ()

# ====== Shared ======
add_subdirectory(shared)

# ====== Server ======
add_subdirectory(server)

# ====== Client ======
add_subdirectory(client)

add_custom_target(StalkerAll DEPENDS StalkerServer StalkerClient)

enable_gcc_flags(StalkerServer)
enable_gcc_flags(StalkerClient)