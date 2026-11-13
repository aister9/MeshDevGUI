#include "workspace/MeshWorkspace.h"
#include "geometry/Object_t.h"
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <stdexcept>

namespace MeshDev
{
std::vector<MeshId> MeshWorkspace::load(const std::filesystem::path &path)
{
    // Resolve once so labels, exports and input protection keep referring to
    // the same file even if the process working directory changes later.
    const auto inputPath = std::filesystem::absolute(path).lexically_normal();
    if (!std::filesystem::is_regular_file(inputPath))
        throw std::runtime_error("Mesh file not found: " + inputPath.u8string());
    Object_t object(inputPath.string());
    if (!object.model || object.model->meshes.empty()) throw std::runtime_error("File has no meshes");
    std::vector<MeshId> added;
    const auto previousSelection = m_selected;
    try
    {
        for (size_t i = 0; i < object.model->meshes.size(); ++i)
        {
            const auto *mesh = object.model->meshes[i];
            if (!mesh || mesh->index.empty()) continue;
            std::string name = inputPath.filename().u8string();
            if (object.model->meshes.size() > 1) name += " / " + std::to_string(i+1);
            added.push_back(addMesh(*mesh, name, inputPath));
        }
        if (added.empty()) throw std::runtime_error("File has no triangle meshes");
    }
    catch (...)
    {
        for (auto id : added) remove(id);
        m_selected = previousSelection;
        throw;
    }
    return added;
}

std::filesystem::path MeshWorkspace::save(MeshId id, std::filesystem::path path)
{
    auto &entry = require(id);
    if (path.empty() || path.filename().empty()) throw std::invalid_argument("Enter an output filename");
    path.replace_extension(".obj");
    for (const auto &input : m_entries)
        if (!input->inputPath.empty() && (std::filesystem::weakly_canonical(path) == std::filesystem::weakly_canonical(input->inputPath)
            || (std::filesystem::exists(path) && std::filesystem::equivalent(path, input->inputPath))))
            throw std::invalid_argument("Choose a different filename from the loaded input meshes");
    const auto vertexColors = colors(id);
    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot open output: " + path.u8string());
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<float>::max_digits10);
    out << "# MeshDevGUI vertex colors\n";
    for (size_t i = 0; i < entry.mesh.vertex.size(); ++i)
    {
        const auto &v = entry.mesh.vertex[i];
        const auto &c = vertexColors[i];
        out << "v " << v.x << ' ' << v.y << ' ' << v.z << ' ' << c.x << ' ' << c.y << ' ' << c.z << '\n';
    }
    for (const auto &f : entry.mesh.index)
        out << "f " << f.x+1 << ' ' << f.y+1 << ' ' << f.z+1 << '\n';
    out.close();
    if (!out) throw std::runtime_error("Failed while writing output: " + path.u8string());
    entry.lastExport = path;
    entry.savedRevision = entry.revision;
    return path;
}
}
