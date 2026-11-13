add_library(meshWorkspaceLib STATIC
    ../../include/workspace/MeshWorkspace.cpp
    ../../include/workspace/MeshWorkspaceIO.cpp
    ../../include/workspace/DeviationMaterial.cpp
)
target_include_directories(meshWorkspaceLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(meshWorkspaceLib PUBLIC geometryLib)
