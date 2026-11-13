add_library(meshRenderHeaders INTERFACE)
target_include_directories(meshRenderHeaders INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(meshRenderHeaders INTERFACE owl::owl_static)

embed_ptx(
    OUTPUT_TARGET meshRendererPTX
    SOURCES ../../include/render/OwlPrograms.cu
    EMBEDDED_SYMBOL_NAMES meshRenderer_ptx
    PTX_LINK_LIBRARIES meshRenderHeaders
)

add_library(meshRendererLib STATIC
    ../../include/render/OrbitCamera.cpp
    ../../include/render/OwlRenderer.cpp
)
target_include_directories(meshRendererLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(meshRendererLib PUBLIC CUDA::cudart PRIVATE owl::owl_static meshRendererPTX)
