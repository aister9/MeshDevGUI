add_library(visualizerLib
    UI/FileDialog.cpp
)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

target_link_libraries(visualizerLib PUBLIC
    imgui::imgui
    glfw
    GLEW::GLEW
    OpenGL::GL
)

target_include_directories(visualizerLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)