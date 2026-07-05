find_package(Protobuf CONFIG REQUIRED)

add_library(SharedProtocol SHARED)

target_sources(SharedProtocol
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/message.proto
)

set(SHARED_PROTOCOL_GENERATED_DIR
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

protobuf_generate(
        TARGET SharedProtocol
        LANGUAGE cpp
        PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/message.proto
        IMPORT_DIRS ${CMAKE_SOURCE_DIR}/shared/src
        PROTOC_OUT_DIR ${SHARED_PROTOCOL_GENERATED_DIR}
)

target_link_libraries(SharedProtocol
        PUBLIC
        protobuf::libprotobuf
)

target_include_directories(SharedProtocol
        PUBLIC
        ${SHARED_PROTOCOL_GENERATED_DIR}
        ${PROJECT_SOURCE_DIR}/shared/src
)

set_target_properties(SharedProtocol PROPERTIES
        WINDOWS_EXPORT_ALL_SYMBOLS ON
)