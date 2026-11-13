cmake_minimum_required (VERSION 3.24)

if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES 75 CACHE STRING "CUDA architectures")
endif()
project ("MeshDevGUI" LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

if(MSVC)
    # CUDA 13's CCCL headers require the conforming MSVC preprocessor.
    # Source files and NVIDIA headers use UTF-8, including on Korean Windows.
    add_compile_options(
        "$<$<COMPILE_LANGUAGE:C,CXX>:/utf-8>"
        "$<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>"
        "$<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/utf-8,/Zc:preprocessor>"
    )
endif()

option(MESHDEV_BUILD_GUI "Build the ImGUIAX GUI and OWL renderer" ON)
option(MESHDEV_BUILD_RENDERER_TESTS "Build GPU renderer smoke tests" OFF)

find_package(CUDAToolkit 11.8 REQUIRED)

# CUDA 13 places Windows runtime DLLs under bin/x64; earlier versions use bin.
# FindCUDAToolkit exposes the import library, not the runtime DLL location.
if(WIN32)
    file(GLOB MESHDEV_CUDA_RUNTIME_DLLS
        "${CUDAToolkit_BIN_DIR}/cudart64_*.dll"
        "${CUDAToolkit_BIN_DIR}/x64/cudart64_*.dll")
    if(NOT MESHDEV_CUDA_RUNTIME_DLLS)
        message(FATAL_ERROR "CUDA runtime DLL not found under ${CUDAToolkit_BIN_DIR}")
    endif()
endif()
function(meshdev_copy_cuda_runtime target)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${MESHDEV_CUDA_RUNTIME_DLLS} "$<TARGET_FILE_DIR:${target}>"
            COMMAND_EXPAND_LISTS VERBATIM)
    endif()
endfunction()

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

if(MESHDEV_BUILD_GUI OR MESHDEV_BUILD_RENDERER_TESTS)
    # Use the same OptiX headers as geometryLib and the legacy OptiX helper.
    set(OptiX_ROOT_DIR "${optix_SOURCE_DIR}")
    set(OWL_DISABLE_TBB ON)
    FetchContent_Declare(owl
        GIT_REPOSITORY https://github.com/NVIDIA/OWL.git
        GIT_TAG a701aecccfb6a6081f3b114bcabb1640b6d39ab0
        SOURCE_SUBDIR meshdev-populate-only
    )
    # Populate first, then opt into only the required subproject targets.
    FetchContent_MakeAvailable(owl)
    add_subdirectory("${owl_SOURCE_DIR}" "${owl_BINARY_DIR}" EXCLUDE_FROM_ALL)
endif()

if(MESHDEV_BUILD_GUI)
    FetchContent_Declare(
        ImGUIAX
        GIT_REPOSITORY https://github.com/aister9/ImGUIAX.git
        GIT_TAG        main
        SOURCE_SUBDIR meshdev-populate-only
    )
    FetchContent_MakeAvailable(ImGUIAX)

    # Pull in ImGUIAX's third-party stack and expose only the Visualizer library.
    include("${imguiax_SOURCE_DIR}/cmake/Dependencies.cmake")
    add_subdirectory(
        "${imguiax_SOURCE_DIR}/libs/Visualizer"
        "${imguiax_BINARY_DIR}/libs/Visualizer"
        EXCLUDE_FROM_ALL
    )
endif()

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

if(MESHDEV_BUILD_RENDERER_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
