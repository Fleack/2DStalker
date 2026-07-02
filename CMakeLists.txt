cmake_minimum_required(VERSION 3.24)
project(S2D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

option(BUILD_TESTS "Build test executables" OFF)

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

function(copy_runtime_dlls target)
    if (WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target}>
                $<TARGET_FILE_DIR:${target}>
                COMMAND_EXPAND_LISTS
        )
    endif ()
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

FetchContent_MakeAvailable(json spdlog asio fmt)

if (BUILD_TESTS)
    include(EnableTesting.cmake)
endif ()

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
            _WIN32_WINNT=0x0A00
    )
endif ()

# ====== Shared ======
add_subdirectory(shared)

# ====== Server ======
add_subdirectory(server)

# ====== Client ======
add_subdirectory(client)

add_custom_target(S2DAll DEPENDS Server Client)
