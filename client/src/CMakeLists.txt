# ==== SFML ====
FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.1.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
)
FetchContent_MakeAvailable(SFML)

# ==== TGUI ====
set(TGUI_BACKEND SFML_GRAPHICS)
set(TGUI_BUILD_GUI_BUILDER OFF)
set(TGUI_CXX_STANDARD 20)

FetchContent_Declare(
        TGUI
        GIT_REPOSITORY https://github.com/texus/TGUI.git
        GIT_TAG v1.13.0
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM
)
FetchContent_MakeAvailable(TGUI)

set_target_properties(tgui PROPERTIES SYSTEM TRUE)

# ==================================

add_subdirectory(network)
add_subdirectory(render)
add_subdirectory(ui)

add_library(ClientApplication
        Application.cpp
        states/ClientConsoleState.cpp
        states/StateManager.cpp
)

target_include_directories(ClientApplication
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ClientApplication
        PUBLIC
        ClientNetwork
        ClientRender
        ClientUi
)

enable_gcc_flags(ClientApplication)

add_executable(Client
        main.cpp
)

target_link_libraries(Client
        PRIVATE
        ClientApplication
        SharedLogger
)

copy_runtime_dlls(Client)

enable_gcc_flags(Client)
