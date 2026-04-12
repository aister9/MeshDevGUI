cmake_minimum_required (VERSION 3.8)

project ("MeshDevGUI" LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_ARCHITECTURES 75)

find_package(CUDAToolkit 11.8 REQUIRED)

#OPTIX
include(FetchContent)
add_definitions(-DBYTE=uint8_t)
FetchContent_Declare(
        optix
        GIT_REPOSITORY https://github.com/NVIDIA/optix-dev.git
        GIT_TAG v7.4.0
)
FetchContent_MakeAvailable(optix)
set(OptiX_INCLUDE_DIR "${optix_SOURCE_DIR}/include")

FetchContent_Declare(
    cuBQL
    GIT_REPOSITORY https://github.com/NVIDIA/cuBQL.git
    GIT_TAG        main     # 또는 특정 commit hash
)

FetchContent_MakeAvailable(cuBQL)

FetchContent_Declare(
    ImGUIAX
    GIT_REPOSITORY https://github.com/aister9/ImGUIAX.git
    GIT_TAG        main
)

FetchContent_GetProperties(ImGUIAX)
if(NOT imguiax_POPULATED)
    FetchContent_Populate(ImGUIAX)
endif()

# Pull in ImGUIAX's third-party stack and expose only the Visualizer library.
include("${imguiax_SOURCE_DIR}/cmake/Dependencies.cmake")
add_subdirectory(
    "${imguiax_SOURCE_DIR}/libs/Visualizer"
    "${imguiax_BINARY_DIR}/libs/Visualizer"
    EXCLUDE_FROM_ALL
)

add_library(optix_common INTERFACE)
target_include_directories(optix_common INTERFACE
    ${CUDAToolkit_INCLUDE_DIRS}
    ${OptiX_INCLUDE_DIR}
)
target_link_libraries(optix_common INTERFACE
    CUDA::cudart
    CUDA::cuda_driver
)

add_subdirectory("libs")
add_subdirectory("demo")
