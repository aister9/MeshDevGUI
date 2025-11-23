cmake_minimum_required (VERSION 3.8)

add_executable (MeshDevConsole "demo.cpp")


set_target_properties(MeshDevConsole PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../output
)

target_link_libraries(MeshDevConsole PRIVATE
    optixBaseLib
    geometryLib
)

target_include_directories(MeshDevConsole PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/libs
     ${CMAKE_CURRENT_SOURCE_DIR}/libs/geometry
     ${cuBQL_SOURCE_DIR}
)

# Copy required runtime DLLs (e.g., cuBQL CUDA builders) next to the executable after build
add_custom_command(TARGET MeshDevConsole POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:cuBQL_cuda_float3>"
        "$<TARGET_FILE_DIR:MeshDevConsole>"
)


find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

add_executable (MeshDevGUI
    "MeshDevGUIDemo.cpp"
)
set_target_properties(MeshDevGUI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../output
)
target_link_libraries(MeshDevGUI PRIVATE
    optixBaseLib
    geometryLib
    visualizerLib
)
target_include_directories(MeshDevGUI PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/libs
     ${CMAKE_CURRENT_SOURCE_DIR}/libs/geometry
     ${cuBQL_SOURCE_DIR}
)
add_custom_command(TARGET MeshDevGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:cuBQL_cuda_float3>"
        "$<TARGET_FILE_DIR:MeshDevGUI>"
)
