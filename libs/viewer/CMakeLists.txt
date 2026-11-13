add_library(meshViewerLib STATIC
    ../../include/viewer/SceneView.cpp
    ../../include/viewer/FrameImage.cpp
    ../../include/viewer/MeshListPanel.cpp
    ../../include/viewer/DeviationPanel.cpp
    ../../include/viewer/MaterialPanel.cpp
)
target_include_directories(meshViewerLib PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${imguiax_SOURCE_DIR}/libs"
)
target_link_libraries(meshViewerLib PUBLIC meshRendererLib meshWorkspaceLib visualizerLib)
