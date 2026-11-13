#pragma once

#include "geometry/TriangleMesh.h"
#include "render/OrbitCamera.h"
#include <cstdint>
#include <memory>

namespace MeshDev
{
struct RenderedFrame
{
    // Owned by the backend, valid until its next render or destruction.
    const std::uint32_t *rgba = nullptr;
    int width = 0;
    int height = 0;
};

class MeshRenderer
{
public:
    virtual ~MeshRenderer() = default;
    virtual const char *name() const = 0;
    virtual void setMesh(const TriangleMesh &mesh, const std::vector<float3> &colors) = 0;
    virtual void setColors(const std::vector<float3> &colors) = 0;
    virtual RenderedFrame render(const CameraFrame &camera, int width, int height, bool lighting) = 0;
};

// Creation is lazy in the viewer so a renderer failure does not disable deviation/export.
std::unique_ptr<MeshRenderer> createOwlRenderer();
}
