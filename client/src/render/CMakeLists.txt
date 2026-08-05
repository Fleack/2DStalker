add_library(ClientRender
        RenderComponent.cpp
)

target_include_directories(ClientRender
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ClientRender
        PUBLIC
        SFML::Graphics
)

enable_gcc_flags(ClientRender)