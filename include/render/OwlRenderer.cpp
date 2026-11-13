#include "render/MeshRenderer.h"
#include "render/OwlPrograms.h"
#include <owl/owl.h>
#include <cuda_runtime_api.h>
#include <cmath>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>

extern "C" const std::uint8_t meshRenderer_ptx[];

namespace MeshDev
{
namespace
{
void checkCuda(cudaError_t result)
{
    if (result != cudaSuccess)
        throw std::runtime_error(std::string("OWL render: ") + cudaGetErrorString(result));
}

struct OwlState
{
    OWLContext context = nullptr;
    OWLRayGen rayGen = nullptr;
    OWLBuffer frameBuffer = nullptr;
    OWLBuffer colors = nullptr;
    size_t vertexCount = 0;
    int width = 0;
    int height = 0;

    ~OwlState()
    {
        // OWL's context owns all handles, including partially constructed scenes.
        if (context)
        {
            try { owlContextDestroy(context); }
            catch (...) { /* Destructors must not throw during failure recovery. */ }
        }
    }
};

class OwlRenderer final : public MeshRenderer
{
public:
    const char *name() const override { return "OWL / OptiX"; }

    void setMesh(const TriangleMesh &mesh, const std::vector<float3> &colors) override
    {
        if (mesh.vertex.empty() || mesh.index.empty())
            throw std::invalid_argument("Preview requires vertices and triangles");
        if (mesh.vertex.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
            throw std::invalid_argument("Mesh exceeds renderer index capacity");
        if (colors.size() != mesh.vertex.size())
            throw std::invalid_argument("Preview color count does not match vertex count");
        for (size_t i = 0; i < mesh.vertex.size(); ++i)
        {
            const auto &v = mesh.vertex[i];
            const auto &c = colors[i];
            if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)
                || !std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.z))
                throw std::invalid_argument("Preview mesh contains non-finite positions or colors");
        }
        for (const auto &f : mesh.index)
            if (f.x >= mesh.vertex.size() || f.y >= mesh.vertex.size() || f.z >= mesh.vertex.size())
                throw std::invalid_argument("Preview mesh contains an invalid triangle index");

        // Stage a complete replacement: an upload/build failure preserves the old scene.
        auto next = std::make_unique<OwlState>();
        next->context = owlContextCreate(nullptr, 1);
        if (!next->context)
            throw std::runtime_error("Could not create an OWL context on CUDA device 0");
        OWLModule module = owlModuleCreate(next->context, reinterpret_cast<const char *>(meshRenderer_ptx));
        OWLVarDecl geometryVars[] = {
            {"vertices", OWL_BUFPTR, OWL_OFFSETOF(OwlMeshData, vertices)},
            {"indices", OWL_BUFPTR, OWL_OFFSETOF(OwlMeshData, indices)},
            {"colors", OWL_BUFPTR, OWL_OFFSETOF(OwlMeshData, colors)}
        };
        OWLGeomType type = owlGeomTypeCreate(next->context, OWL_TRIANGLES, sizeof(OwlMeshData), geometryVars, std::size(geometryVars));
        owlGeomTypeSetClosestHit(type, 0, module, "meshClosestHit");

        static_assert(sizeof(float3) == 3*sizeof(float), "Position layout mismatch");
        static_assert(sizeof(uint3) == 3*sizeof(int), "Index layout mismatch");
        OWLBuffer vertices = owlDeviceBufferCreate(next->context, OWL_FLOAT3, mesh.vertex.size(), mesh.vertex.data());
        OWLBuffer indices = owlDeviceBufferCreate(next->context, OWL_INT3, mesh.index.size(), mesh.index.data());
        OWLBuffer vertexColors = owlDeviceBufferCreate(next->context, OWL_FLOAT3, colors.size(), colors.data());
        next->colors = vertexColors;
        next->vertexCount = colors.size();
        OWLGeom geom = owlGeomCreate(next->context, type);
        owlTrianglesSetVertices(geom, vertices, mesh.vertex.size(), sizeof(float3), 0);
        owlTrianglesSetIndices(geom, indices, mesh.index.size(), sizeof(uint3), 0);
        owlGeomSetBuffer(geom, "vertices", vertices);
        owlGeomSetBuffer(geom, "indices", indices);
        owlGeomSetBuffer(geom, "colors", vertexColors);
        OWLGroup triangles = owlTrianglesGeomGroupCreate(next->context, 1, &geom);
        owlGroupBuildAccel(triangles);
        OWLGroup world = owlInstanceGroupCreate(next->context, 1, &triangles);
        owlGroupBuildAccel(world);
        owlMissProgCreate(next->context, module, "meshMiss", 0, nullptr, 0);

        OWLVarDecl rayVars[] = {
            {"pixels", OWL_BUFPTR, OWL_OFFSETOF(OwlRayGenData, pixels)},
            {"size", OWL_INT2, OWL_OFFSETOF(OwlRayGenData, size)},
            {"world", OWL_GROUP, OWL_OFFSETOF(OwlRayGenData, world)},
            {"position", OWL_FLOAT3, OWL_OFFSETOF(OwlRayGenData, position)},
            {"direction00", OWL_FLOAT3, OWL_OFFSETOF(OwlRayGenData, direction00)},
            {"horizontal", OWL_FLOAT3, OWL_OFFSETOF(OwlRayGenData, horizontal)},
            {"vertical", OWL_FLOAT3, OWL_OFFSETOF(OwlRayGenData, vertical)},
            {"lighting", OWL_INT, OWL_OFFSETOF(OwlRayGenData, lighting)}
        };
        next->rayGen = owlRayGenCreate(next->context, module, "meshRayGen", sizeof(OwlRayGenData), rayVars, std::size(rayVars));
        next->frameBuffer = owlHostPinnedBufferCreate(next->context, OWL_UINT, 1);
        owlRayGenSetBuffer(next->rayGen, "pixels", next->frameBuffer);
        owlRayGenSetGroup(next->rayGen, "world", world);
        owlRayGenSet2i(next->rayGen, "size", owl2i{1,1});
        owlRayGenSet3f(next->rayGen, "position", owl3f{0,0,1});
        owlRayGenSet3f(next->rayGen, "direction00", owl3f{0,0,-1});
        owlRayGenSet3f(next->rayGen, "horizontal", owl3f{1,0,0});
        owlRayGenSet3f(next->rayGen, "vertical", owl3f{0,-1,0});
        owlRayGenSet1i(next->rayGen, "lighting", 0);
        owlBuildPrograms(next->context);
        owlBuildPipeline(next->context);
        owlBuildSBT(next->context);
        m_state = std::move(next);
    }

    void setColors(const std::vector<float3> &colors) override
    {
        if (!m_state || colors.size() != m_state->vertexCount)
            throw std::invalid_argument("Color count does not match the displayed mesh");
        for (const auto &c : colors)
            if (!std::isfinite(c.x) || !std::isfinite(c.y) || !std::isfinite(c.z))
                throw std::invalid_argument("Non-finite vertex color");
        // Keep the triangle BVH, camera and framebuffer when changing a material.
        owlBufferUpload(m_state->colors, colors.data());
    }

    RenderedFrame render(const CameraFrame &camera, int width, int height, bool lighting) override
    {
        if (!m_state)
            return {};
        if (width < 1 || height < 1 || width > 4096 || height > 4096)
            throw std::invalid_argument("Preview resolution must be between 1 and 4096");
        auto &s = *m_state;
        if (width != s.width || height != s.height)
        {
            owlBufferResize(s.frameBuffer, static_cast<size_t>(width)*height);
            s.width = width;
            s.height = height;
        }
        owlRayGenSet2i(s.rayGen, "size", owl2i{width,height});
        const auto setPoint = [&](const char *name, const float3 &p) {
            owlRayGenSet3f(s.rayGen, name, owl3f{p.x,p.y,p.z});
        };
        setPoint("position", camera.position);
        setPoint("direction00", camera.direction00);
        setPoint("horizontal", camera.horizontal);
        setPoint("vertical", camera.vertical);
        owlRayGenSet1i(s.rayGen, "lighting", lighting ? 1 : 0);
        owlBuildSBT(s.context, OWL_SBT_RAYGENS);
        owlRayGenLaunch2D(s.rayGen, width, height);
        // Legacy OWL raygen launches are asynchronous; host/OpenGL must not read early.
        checkCuda(cudaDeviceSynchronize());
        return {static_cast<const std::uint32_t *>(owlBufferGetPointer(s.frameBuffer,0)), width, height};
    }

private:
    std::unique_ptr<OwlState> m_state;
};
}

std::unique_ptr<MeshRenderer> createOwlRenderer()
{
    return std::make_unique<OwlRenderer>();
}
}
