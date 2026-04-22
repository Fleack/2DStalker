set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/message.proto)
set(PROTO_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)

add_custom_command(
        OUTPUT ${PROTO_OUT_DIR}/message.pb.cc ${PROTO_OUT_DIR}/message.pb.h
        COMMAND ${CMAKE_COMMAND} -E make_directory ${PROTO_OUT_DIR}
        COMMAND protobuf::protoc
        --proto_path=${CMAKE_CURRENT_SOURCE_DIR}
        --cpp_out=${PROTO_OUT_DIR}
        ${PROTO_FILE}
        DEPENDS ${PROTO_FILE}
        VERBATIM
)

add_library(StalkerProtocol STATIC ${PROTO_OUT_DIR}/message.pb.cc)
target_include_directories(StalkerProtocol PUBLIC ${PROTO_OUT_DIR})
target_link_libraries(StalkerProtocol PUBLIC protobuf::libprotobuf)