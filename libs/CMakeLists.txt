add_library(optixBaseLib
    ../include/OptiX_Base.cpp
)
target_link_libraries(optixBaseLib PUBLIC
    optix_common
)
target_include_directories(optixBaseLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../include
)

add_subdirectory(geometry)

if(MESHDEV_BUILD_GUI OR MESHDEV_BUILD_RENDERER_TESTS)
    add_subdirectory(workspace)
    add_subdirectory(render)
endif()
if(MESHDEV_BUILD_GUI)
    add_subdirectory(viewer)
    add_subdirectory(app)
endif()
