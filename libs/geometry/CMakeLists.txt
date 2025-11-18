add_library(geometryLib
    ../../include/geometry/Object_t.cpp
    ../../include/geometry/GeometryDeviationHost.cpp
    ../../include/geometry/GeometryDeviationDevice.cu
)
target_link_libraries(geometryLib PUBLIC
    cuBQL_cuda_float3
    optix_common
)
target_include_directories(geometryLib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../../libs
    ../../include/geometry
    ${cuBQL_SOURCE_DIR}
)
set_source_files_properties(
    ../../include/geometry/GeometryDeviationDevice.cu
    PROPERTIES LANGUAGE CUDA
)
