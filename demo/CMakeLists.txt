cmake_minimum_required (VERSION 3.8)

add_executable (MeshDevGUI "demo.cpp")
set_target_properties(MeshDevGUI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../output
)

target_link_libraries(MeshDevGUI PRIVATE
    optixBaseLib
    geometryLib
)

target_include_directories(MeshDevGUI PUBLIC
     ${CMAKE_CURRENT_SOURCE_DIR}/libs
     ${CMAKE_CURRENT_SOURCE_DIR}/libs/geometry
     ${cuBQL_SOURCE_DIR}
)

# Copy required runtime DLLs (e.g., cuBQL CUDA builders) next to the executable after build
add_custom_command(TARGET MeshDevGUI POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:cuBQL_cuda_float3>"
        "$<TARGET_FILE_DIR:MeshDevGUI>"
)
