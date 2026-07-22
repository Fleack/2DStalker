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

# Boost.Asio from Boost 1.92.0 beta 1
FetchContent_Declare(
        Boost
        URL https://archives.boost.io/beta/1.92.0.beta1/source/boost_1_92_0_b1.tar.gz
        URL_HASH SHA256=585e0a028cc3f1f7711842b7cbfbb5faece2f243ef57ffe7fb846e9b61e2faa5
        SYSTEM
        EXCLUDE_FROM_ALL
)

# Fmt
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 12.1.0
)

FetchContent_MakeAvailable(json spdlog Boost fmt)

add_library(boost_asio_headers INTERFACE)
add_library(Boost::asio ALIAS boost_asio_headers)

target_include_directories(boost_asio_headers SYSTEM INTERFACE
        ${boost_SOURCE_DIR}
)

if (BUILD_TESTS)
    include(EnableTesting.cmake)
endif ()

# ====== Json headers ======
add_library(json_headers INTERFACE)

target_include_directories(json_headers SYSTEM INTERFACE
        ${json_SOURCE_DIR}/include
)

# ====== Shared ======
add_subdirectory(shared)

# ====== Server ======
add_subdirectory(server)

# ====== Client ======
add_subdirectory(client)

add_custom_target(S2DAll DEPENDS Server Client)
