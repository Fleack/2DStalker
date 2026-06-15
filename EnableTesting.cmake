FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.13.0
)
FetchContent_MakeAvailable(Catch2)

include(CTest)

function(s2d_add_test target_name)
    cmake_parse_arguments(
            ARG
            ""
            ""
            "SOURCES;LIBS"
            ${ARGN}
    )

    add_executable(${target_name}
            ${ARG_SOURCES}
    )

    target_link_libraries(${target_name} PRIVATE
            Catch2::Catch2WithMain
            ServerNetwork
            Shared
            ${ARG_LIBS}
    )

    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()