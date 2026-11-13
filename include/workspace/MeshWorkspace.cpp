#include "workspace/MeshWorkspace.h"
#include "geometry/GeometryDeviation.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace MeshDev
{
const MeshEntry *MeshWorkspace::find(MeshId id) const
{
    for (const auto &entry : m_entries) if (entry->id == id) return entry.get();
    return nullptr;
}
MeshEntry &MeshWorkspace::require(MeshId id)
{
    for (auto &entry : m_entries) if (entry->id == id) return *entry;
    throw std::invalid_argument("Select a mesh from Mesh List");
}

MeshId MeshWorkspace::addMesh(TriangleMesh mesh, std::string name, std::filesystem::path path)
{
    if (mesh.vertex.empty() || mesh.index.empty()) throw std::invalid_argument("Mesh has no triangles");
    for (const auto &v : mesh.vertex)
        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z))
            throw std::invalid_argument("Mesh has non-finite vertices");
    std::vector<float> edges;
    edges.reserve(mesh.index.size()*3);
    double sum = 0;
    for (const auto &f : mesh.index)
    {
        if (f.x >= mesh.vertex.size() || f.y >= mesh.vertex.size() || f.z >= mesh.vertex.size())
            throw std::invalid_argument("Mesh has an invalid triangle index");
        for (float e : {length(mesh.vertex[f.y]-mesh.vertex[f.x]),
                        length(mesh.vertex[f.z]-mesh.vertex[f.y]),
                        length(mesh.vertex[f.x]-mesh.vertex[f.z])})
        {
            if (!std::isfinite(e)) throw std::invalid_argument("Mesh edge length exceeds float range");
            edges.push_back(e);
            sum += e;
        }
    }
    auto entry = std::make_unique<MeshEntry>();
    entry->id = m_nextId++;
    entry->name = std::move(name);
    entry->inputPath = std::move(path);
    entry->mesh = std::move(mesh);
    entry->meanEdge = static_cast<float>(sum / edges.size());
    auto middle = edges.begin() + edges.size()/2;
    std::nth_element(edges.begin(), middle, edges.end());
    entry->medianEdge = *middle;
    const auto id = entry->id;
    m_entries.push_back(std::move(entry));
    m_selected = id;
    return id;
}

void MeshWorkspace::select(MeshId id)
{
    if (id != NoMesh) require(id);
    m_selected = id;
}
void MeshWorkspace::remove(MeshId id)
{
    require(id);
    m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
        [id](const auto &entry) { return entry->id == id; }), m_entries.end());
    if (m_selected == id) m_selected = NoMesh;
    if (m_source == id) m_source = NoMesh;
    if (m_target == id) m_target = NoMesh;
}
void MeshWorkspace::setSource(MeshId id)
{
    require(id);
    m_source = id;
    if (m_target == id) m_target = NoMesh;
}
void MeshWorkspace::setTarget(MeshId id)
{
    require(id);
    m_target = id;
    if (m_source == id) m_source = NoMesh;
}
bool MeshWorkspace::canCompute() const
{
    return m_source != m_target && find(m_source) && find(m_target);
}

void MeshWorkspace::compute(ComputeMode mode)
{
    if (!canCompute()) throw std::invalid_argument("Choose different Source and Target meshes");
    if (mode != ComputeMode::CPU && mode != ComputeMode::GPU && mode != ComputeMode::Both)
        throw std::invalid_argument("Unknown compute mode");
    const auto &source = require(m_source);
    auto &target = require(m_target);
    std::vector<DeviationResult> results;
    auto run = [&](auto &calculator, const char *backend) {
        calculator.computeDeviation();
        DeviationResult result;
        result.source = source.id;
        result.sourceName = source.name;
        result.backend = backend;
        result.distances = calculator.getDeviations();
        if (result.distances.size() != target.mesh.vertex.size())
            throw std::runtime_error("Deviation result size mismatch");
        result.minimum = INFINITY;
        for (float d : result.distances)
        {
            if (!std::isfinite(d) || d < 0) throw std::runtime_error("Invalid deviation result");
            result.minimum = std::min(result.minimum, d);
            result.maximum = std::max(result.maximum, d);
            result.mean += d / static_cast<double>(result.distances.size());
        }
        results.push_back(std::move(result));
    };
    if (mode == ComputeMode::CPU || mode == ComputeMode::Both)
    {
        GeometryDeviation<SPIN::ExecTag::HOST> calculator(source.mesh, target.mesh);
        run(calculator, "CPU");
    }
    if (mode == ComputeMode::GPU || mode == ComputeMode::Both)
    {
        GeometryDeviation<SPIN::ExecTag::DEVICE> calculator(source.mesh, target.mesh);
        run(calculator, "GPU");
    }
    // Commit only after every requested calculation succeeds. No encoding or file I/O here.
    target.results = std::move(results);
    target.activeResult = target.results.size()-1;
    ++target.revision;
    m_selected = target.id;
}

void MeshWorkspace::setMaterial(MeshId id, const DeviationMaterial &material)
{
    validateMaterial(material);
    auto &entry = require(id);
    const float sigma = (material.sigmaMethod == SigmaMethod::MedianEdge ? entry.medianEdge : entry.meanEdge)
                        * material.sigmaScale;
    if (entry.result() && material.showDeviation && (!std::isfinite(sigma) || sigma <= 0))
        throw std::invalid_argument("Color mapping requires a positive finite sigma");
    entry.material = material;
    ++entry.revision;
}
void MeshWorkspace::selectResult(MeshId id, size_t index)
{
    auto &entry = require(id);
    if (index >= entry.results.size()) throw std::invalid_argument("Unknown deviation result");
    entry.activeResult = index;
    ++entry.revision;
}
std::vector<float3> MeshWorkspace::colors(MeshId id) const
{
    const auto *entry = find(id);
    if (!entry) throw std::invalid_argument("Select a mesh from Mesh List");
    if (entry->material.showDeviation && entry->result())
        return encodeDeviation(entry->result()->distances, entry->material, entry->sigma());
    return std::vector<float3>(entry->mesh.vertex.size(), entry->material.baseColor);
}
}
