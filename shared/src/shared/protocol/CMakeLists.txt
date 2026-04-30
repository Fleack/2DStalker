set(PROTO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(GEN_ROOT ${CMAKE_CURRENT_BINARY_DIR}/generated)

add_custom_command(
        OUTPUT
        ${GEN_ROOT}/shared/protocol/message.pb.cc
        ${GEN_ROOT}/shared/protocol/message.pb.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${GEN_ROOT}/shared/protocol
        COMMAND protobuf::protoc
        -I ${PROTO_ROOT}
        --cpp_out=${GEN_ROOT}
        ${PROTO_ROOT}/shared/protocol/message.proto
        DEPENDS ${PROTO_ROOT}/shared/protocol/message.proto
        VERBATIM
)

add_library(StalkerProtocol STATIC
        ${GEN_ROOT}/shared/protocol/message.pb.cc
)

target_include_directories(StalkerProtocol PUBLIC
        ${GEN_ROOT}
)

target_link_libraries(StalkerProtocol PUBLIC
        protobuf::libprotobuf
)