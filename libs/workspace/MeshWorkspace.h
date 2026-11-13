#pragma once
#include "workspace/DeviationMaterial.h"
#include <cstdint>
#include <filesystem>
#include <memory>

namespace MeshDev
{
using MeshId = std::uint64_t;
constexpr MeshId NoMesh = 0;
enum class ComputeMode { CPU, GPU, Both };

struct DeviationResult
{
    MeshId source = NoMesh;
    std::string sourceName;
    std::string backend;
    std::vector<float> distances; // Raw distances; material edits never change these.
    float minimum = 0, maximum = 0;
    double mean = 0;
};

struct MeshEntry
{
    MeshId id = NoMesh;
    std::string name;
    std::filesystem::path inputPath;
    TriangleMesh mesh;
    float medianEdge = 0, meanEdge = 0;
    std::vector<DeviationResult> results;
    size_t activeResult = 0;
    DeviationMaterial material;
    std::uint64_t revision = 1, savedRevision = 0;
    std::filesystem::path lastExport;

    const DeviationResult *result() const
    { return activeResult < results.size() ? &results[activeResult] : nullptr; }
    float sigma() const
    { return (material.sigmaMethod == SigmaMethod::MedianEdge ? medianEdge : meanEdge) * material.sigmaScale; }
};

// Owns meshes, role assignments, raw results and per-mesh materials. No UI/renderer dependency.
class MeshWorkspace
{
public:
    const std::vector<std::unique_ptr<MeshEntry>> &entries() const { return m_entries; }
    const MeshEntry *find(MeshId id) const;
    const MeshEntry *selected() const { return find(m_selected); }
    MeshId source() const { return m_source; }
    MeshId target() const { return m_target; }
    MeshId addMesh(TriangleMesh mesh, std::string name, std::filesystem::path inputPath = {});
    std::vector<MeshId> load(const std::filesystem::path &path);
    void select(MeshId id);
    void remove(MeshId id);
    void setSource(MeshId id);
    void setTarget(MeshId id);
    bool canCompute() const;
    void compute(ComputeMode mode);
    void setMaterial(MeshId id, const DeviationMaterial &material);
    void selectResult(MeshId id, size_t index);
    std::vector<float3> colors(MeshId id) const;
    std::filesystem::path save(MeshId id, std::filesystem::path path);

private:
    MeshEntry &require(MeshId id);
    std::vector<std::unique_ptr<MeshEntry>> m_entries;
    MeshId m_nextId = 1, m_selected = NoMesh, m_source = NoMesh, m_target = NoMesh;
};
}
