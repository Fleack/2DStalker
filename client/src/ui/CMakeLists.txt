add_library(ClientUi
        ConnectionPanel.cpp
        EventLogPanel.cpp
        Style.cpp
        UiComponent.cpp
)

target_include_directories(ClientUi
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ClientUi
        PUBLIC
        TGUI::TGUI
        SFML::Graphics
)

enable_gcc_flags(ClientUi)