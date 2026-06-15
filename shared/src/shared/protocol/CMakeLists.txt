find_package(Protobuf CONFIG REQUIRED)

add_library(SharedProtocol STATIC)

target_sources(SharedProtocol
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/message.proto
)

protobuf_generate(
        TARGET SharedProtocol
        LANGUAGE cpp
        PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/message.proto
        IMPORT_DIRS ${CMAKE_SOURCE_DIR}/shared/src
        PROTOC_OUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_link_libraries(SharedProtocol
        PUBLIC
        protobuf::libprotobuf
)

target_include_directories(SharedProtocol
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/generated
)

target_include_directories(SharedProtocol INTERFACE ${PROJECT_SOURCE_DIR}/shared/src)