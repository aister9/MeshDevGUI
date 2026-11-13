#include "render/OwlPrograms.h"
#include <owl/owl.h>
#include <optix_device.h>

using namespace owl;
using namespace MeshDev;

struct OwlPayload
{
    vec3f color;
    int lighting;
};

OPTIX_RAYGEN_PROGRAM(meshRayGen)()
{
    const auto &data = getProgramData<OwlRayGenData>();
    const vec2i pixel = getLaunchIndex();
    const vec2f uv = (vec2f(pixel) + vec2f(0.5f)) / vec2f(data.size);
    Ray ray;
    ray.origin = data.position;
    ray.direction = normalize(vec3f(data.direction00) + uv.x*vec3f(data.horizontal) + uv.y*vec3f(data.vertical));
    OwlPayload payload{vec3f(0.0f), data.lighting};
    traceRay(data.world, ray, payload);
    // Store unmodified display-space colors when lighting is disabled.
    data.pixels[pixel.x + data.size.x*pixel.y] = make_rgba(payload.color);
}

OPTIX_CLOSEST_HIT_PROGRAM(meshClosestHit)()
{
    const auto &data = getProgramData<OwlMeshData>();
    auto &payload = getPRD<OwlPayload>();
    const uint3 triangle = data.indices[optixGetPrimitiveIndex()];
    const float2 barycentric = optixGetTriangleBarycentrics();
    payload.color = (1.0f-barycentric.x-barycentric.y)*vec3f(data.colors[triangle.x])
                    + barycentric.x*vec3f(data.colors[triangle.y]) + barycentric.y*vec3f(data.colors[triangle.z]);
    if (payload.lighting)
    {
        const vec3f normal = cross(vec3f(data.vertices[triangle.y])-vec3f(data.vertices[triangle.x]),
                                  vec3f(data.vertices[triangle.z])-vec3f(data.vertices[triangle.x]));
        const float magnitude = sqrtf(dot(normal,normal));
        const float diffuse = magnitude > 0 ? fabsf(dot(vec3f(optixGetWorldRayDirection()),normal)/magnitude) : 0;
        payload.color *= 0.25f + 0.75f*diffuse;
    }
}

OPTIX_MISS_PROGRAM(meshMiss)()
{
    getPRD<OwlPayload>().color = vec3f(0.055f, 0.065f, 0.085f);
}
