# The camera test can run without a GPU/OWL, but needs CUDA headers for helper_math:
# cmake -S tests -B build-camera
cmake_minimum_required(VERSION 3.24)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(MeshDevCameraTests LANGUAGES CXX)
    find_package(CUDAToolkit 11.8 REQUIRED)
    enable_testing()
endif()

if(TARGET meshWorkspaceLib)
    add_executable(MeshDevWorkspaceTest WorkspaceTest.cpp)
    target_link_libraries(MeshDevWorkspaceTest PRIVATE meshWorkspaceLib)
    meshdev_copy_cuda_runtime(MeshDevWorkspaceTest)
    add_custom_command(TARGET MeshDevWorkspaceTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:cuBQL_cuda_float3>" "$<TARGET_FILE_DIR:MeshDevWorkspaceTest>"
        VERBATIM)
    add_test(NAME workspace_cpu COMMAND MeshDevWorkspaceTest)
    add_test(NAME workspace_gpu COMMAND MeshDevWorkspaceTest --gpu)
    set_tests_properties(workspace_gpu PROPERTIES LABELS "gpu;cubql")
endif()

add_executable(MeshDevCameraTest CameraTest.cpp ../include/render/OrbitCamera.cpp)
target_include_directories(MeshDevCameraTest PRIVATE ../libs)
target_link_libraries(MeshDevCameraTest PRIVATE CUDA::toolkit)
target_compile_features(MeshDevCameraTest PRIVATE cxx_std_17)
add_test(NAME camera COMMAND MeshDevCameraTest)

if(TARGET meshRendererLib)
    add_executable(MeshDevRendererSmoke RendererSmoke.cpp)
    target_link_libraries(MeshDevRendererSmoke PRIVATE meshRendererLib)
    meshdev_copy_cuda_runtime(MeshDevRendererSmoke)
    add_test(NAME owl_renderer COMMAND MeshDevRendererSmoke)
    set_tests_properties(owl_renderer PROPERTIES LABELS "gpu;optix")
endif()

if(TARGET geometryLib)
    add_executable(MeshDevFrameImageTest FrameImageTest.cpp ../include/viewer/FrameImage.cpp)
    target_link_libraries(MeshDevFrameImageTest PRIVATE geometryLib)
    meshdev_copy_cuda_runtime(MeshDevFrameImageTest)
    add_custom_command(TARGET MeshDevFrameImageTest POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:cuBQL_cuda_float3>" "$<TARGET_FILE_DIR:MeshDevFrameImageTest>"
        VERBATIM)
    add_test(NAME frame_image COMMAND MeshDevFrameImageTest)
    add_executable(MeshDevGeometrySmoke GeometrySmoke.cpp)
    target_link_libraries(MeshDevGeometrySmoke PRIVATE geometryLib)
    meshdev_copy_cuda_runtime(MeshDevGeometrySmoke)
    add_custom_command(TARGET MeshDevGeometrySmoke POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:cuBQL_cuda_float3>" "$<TARGET_FILE_DIR:MeshDevGeometrySmoke>"
        VERBATIM)
    add_test(NAME geometry_cpu_gpu COMMAND MeshDevGeometrySmoke)
    set_tests_properties(geometry_cpu_gpu PROPERTIES LABELS "gpu;cubql")
endif()
