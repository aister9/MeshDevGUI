#pragma once

#include <vector_types.h>
#include <optix_types.h>
#include <cstdint>

namespace MeshDev
{
struct OwlMeshData
{
    const float3 *vertices;
    const uint3 *indices;
    const float3 *colors;
};

struct OwlRayGenData
{
    std::uint32_t *pixels;
    int2 size;
    OptixTraversableHandle world;
    float3 position;
    float3 direction00;
    float3 horizontal;
    float3 vertical;
    int lighting;
};

}
