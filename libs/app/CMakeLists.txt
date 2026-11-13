add_library(meshAppLib STATIC ../../include/app/MeshDevApplication.cpp)
target_include_directories(meshAppLib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(meshAppLib PUBLIC meshViewerLib)
