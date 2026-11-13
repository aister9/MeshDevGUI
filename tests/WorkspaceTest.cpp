#include "workspace/MeshWorkspace.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

using namespace MeshDev;
namespace fs = std::filesystem;
namespace
{
void require(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
template<typename F> void rejects(F action)
{
    bool failed = false;
    try { action(); } catch (const std::exception &) { failed = true; }
    require(failed, "Invalid operation was accepted");
}
std::string read(const fs::path &p)
{
    std::ifstream in(p);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
size_t fileCount(const fs::path &dir)
{ return static_cast<size_t>(std::distance(fs::directory_iterator(dir), fs::directory_iterator())); }
}

int main(int argc, char **argv)
{
    try
    {
        // Exercise local datasets with the same importer used by Mesh List.
        if (argc > 2 && std::string(argv[1]) == "--load")
        {
            MeshWorkspace imported;
            for (int i = 2; i < argc; ++i)
            {
                const auto ids = imported.load(fs::path(argv[i]));
                for (auto id : ids)
                {
                    const auto *mesh = imported.find(id);
                    require(mesh->inputPath.is_absolute(), "Import did not retain an absolute path");
                    std::cout << mesh->name << ": " << mesh->mesh.vertex.size() << " vertices, "
                              << mesh->mesh.index.size() << " triangles\n";
                }
            }
            return 0;
        }
        const bool gpu = argc > 1 && std::string(argv[1]) == "--gpu";
        const auto dir = fs::current_path() / ("workspace-test-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        fs::create_directory(dir);
        const auto input = dir / "pair.obj";
        const std::string obj = "o source\nv -1 -1 0\nv 1 -1 0\nv 1 1 0\nv -1 1 0\nf 1 2 3\nf 1 3 4\n"
            "o target\nv -1 -1 0.4\nv 1 -1 0.4\nv 1 1 0.4\nv -1 1 0.4\nf 5 6 7\nf 5 7 8\n";
        { std::ofstream out(input); out << obj; }
        MeshWorkspace workspace;
        const auto ids = workspace.load(input);
        require(ids.size() == 2, "Multi-mesh file was not split into list entries");
        require(ids[0] != ids[1], "Mesh IDs must be stable and unique");
        const auto source = ids[0], target = ids[1];
        require(!workspace.canCompute(), "Roles should require explicit assignment");
        workspace.setSource(source);
        workspace.setTarget(target);
        require(workspace.canCompute(), "Assigned roles cannot compute");
        workspace.compute(gpu ? ComputeMode::Both : ComputeMode::CPU);
        const auto *entry = workspace.find(target);
        require(workspace.selected()->id == target, "Calculation did not select target");
        require(entry->results.size() == (gpu ? 2 : 1), "CPU/GPU results not retained separately");
        for (const auto &result : entry->results)
            for (float d : result.distances) require(std::abs(d-.4f) < 1e-5f, "Wrong raw distance");
        const auto raw = entry->result()->distances;
        require(fileCount(dir) == 1 && entry->lastExport.empty(), "Calculation wrote a file");

        auto material = entry->material;
        material.colorMap = ColorMap::Gray;
        workspace.setMaterial(target, material);
        const auto gray = workspace.colors(target);
        require(std::abs(gray[0].x-.2f) < .01f && gray[0].x == gray[0].y && gray[0].y == gray[0].z,
                "Gray mapping does not match analytic distance / median edge");
        material.colorMap = ColorMap::Turbo;
        workspace.setMaterial(target, material);
        const auto turbo = workspace.colors(target);
        require(length(turbo[0]-gray[0]) > .1f, "Color map edit did not change presentation");
        material.sigmaScale = 2;
        workspace.setMaterial(target, material);
        require(length(workspace.colors(target)[0]-turbo[0]) > .01f, "Sigma edit did not change presentation");
        require(entry->result()->distances == raw && fileCount(dir) == 1, "Material edit mutated raw data or wrote a file");
        const auto revision = entry->revision;
        material.sigmaScale = 0;
        rejects([&] { workspace.setMaterial(target, material); });
        require(entry->revision == revision, "Invalid material modified state");
        workspace.select(source);
        workspace.select(target);
        require(entry->material.colorMap == ColorMap::Turbo && entry->material.sigmaScale == 2, "Per-mesh material was lost");
        workspace.selectResult(target, 0);
        require(entry->result()->backend == "CPU", "Cannot select CPU result");

        const auto expectedColors = workspace.colors(target);
        const auto output = workspace.save(target, dir / "colored.ply");
        require(output.extension() == ".obj" && fs::exists(output), "Explicit save did not produce OBJ");
        require(entry->savedRevision == entry->revision, "Save state not recorded");
        std::istringstream saved(read(output));
        std::string line;
        size_t vertices = 0, faces = 0;
        while (std::getline(saved,line))
        {
            std::istringstream row(line);
            std::string tag; row >> tag;
            if (tag == "v")
            {
                float x,y,z,r,g,b; require(bool(row >> x >> y >> z >> r >> g >> b), "Missing OBJ vertex colors");
                require(std::abs(z-.4f) < 1e-6f, "Saved geometry changed");
                require(length(make_float3(r,g,b)-expectedColors.at(vertices)) < 1e-6f, "Saved colors differ from presentation");
                ++vertices;
            }
            if (tag == "f") ++faces;
        }
        require(vertices == 4 && faces == 2, "Export has wrong topology");
        rejects([&] { workspace.save(target,input); });
        require(read(input) == obj, "Save overwrote input mesh");
        material = entry->material;
        material.showDeviation = false;
        material.baseColor = make_float3(.1f,.2f,.3f);
        workspace.setMaterial(target, material);
        require(entry->savedRevision != entry->revision, "Material edit should require another save");
        require(length(workspace.colors(target)[0]-material.baseColor) < 1e-6f, "Base material toggle failed");
        rejects([&] { workspace.save(target,dir/"missing"/"out.obj"); });
        require(entry->savedRevision != entry->revision, "Failed save marked edits saved");
        rejects([&] { workspace.load(dir/"missing.obj"); });
        require(workspace.entries().size() == 2 && workspace.selected()->id == target, "Failed load damaged workspace");
        workspace.remove(source);
        require(!workspace.canCompute() && workspace.source() == NoMesh, "Removed source left dangling role");
        require(entry->result()->sourceName.size() && entry->result()->distances == raw, "Stored result depends on removed source");
        workspace.remove(target);
        require(!workspace.selected() && workspace.target() == NoMesh, "Removed target left dangling selection");

        // Match the GUI's output/Release working directory and ../../ input.
        const auto previousDirectory = fs::current_path();
        fs::create_directories(dir / "output" / "Release");
        struct RestoreDirectory
        {
            fs::path path;
            ~RestoreDirectory() { fs::current_path(path); }
        } restore{previousDirectory};
        fs::current_path(dir / "output" / "Release");
        const auto relativeIds = workspace.load("../../pair.obj");
        require(relativeIds.size() == 2, "Relative OBJ import failed");
        require(workspace.find(relativeIds[0])->inputPath.is_absolute(), "Relative input path was retained");
        require(fs::equivalent(workspace.find(relativeIds[0])->inputPath, input), "Relative import resolved to wrong file");
        fs::current_path(previousDirectory);
        rejects([&] { workspace.save(relativeIds[0],input); });
        require(read(input) == obj, "Changing working directory bypassed input protection");
        std::cout << "List roles, raw results, material edits and explicit export checks passed\n";
        return 0;
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
