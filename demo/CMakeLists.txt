cmake_minimum_required (VERSION 3.24)

add_executable (MeshDevConsole "demo.cpp")
meshdev_copy_cuda_runtime(MeshDevConsole)


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

if(MESHDEV_BUILD_GUI)
add_executable (MeshDevGUI
    "MeshDevGUIDemo.cpp"
)
meshdev_copy_cuda_runtime(MeshDevGUI)
set_target_properties(MeshDevGUI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../output
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:MeshDevGUI>"
)
target_link_libraries(MeshDevGUI PRIVATE
    meshAppLib
)
target_include_directories(MeshDevGUI PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/libs
     ${CMAKE_CURRENT_SOURCE_DIR}/libs/geometry
     ${cuBQL_SOURCE_DIR}
     ${imguiax_SOURCE_DIR}/libs
)
add_custom_command(TARGET MeshDevGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:cuBQL_cuda_float3>"
        "$<TARGET_FILE_DIR:MeshDevGUI>"
)
endif()
