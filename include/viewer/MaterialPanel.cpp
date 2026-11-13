#include "viewer/MaterialPanel.h"
#include <algorithm>
#include <cstdio>
#include <exception>

namespace MeshDev
{
void MaterialPanel::drawMaterial(const MeshEntry &entry)
{
    ImGui::TextWrapped("%s", entry.name.c_str());
    ImGui::Text("%zu vertices / %zu triangles", entry.mesh.vertex.size(), entry.mesh.index.size());
    ImGui::Separator();
    if (!entry.results.empty())
    {
        if (ImGui::BeginCombo("Result", entry.result()->backend.c_str()))
        {
            for (size_t i = 0; i < entry.results.size(); ++i)
                if (ImGui::Selectable(entry.results[i].backend.c_str(), entry.activeResult == i)) m_workspace.selectResult(entry.id, i);
            ImGui::EndCombo();
        }
        const auto &result = *entry.result();
        ImGui::TextWrapped("Calculated against: %s", result.sourceName.c_str());
        ImGui::Text("Raw min / max: %.6g / %.6g", result.minimum, result.maximum);
        ImGui::Text("Raw mean: %.6g", result.mean);
    }
    else ImGui::TextWrapped("No deviation result. Assign Source and Target, then calculate.");
    auto material = entry.material;
    bool changed = false;
    ImGui::BeginDisabled(!entry.result());
    changed |= ImGui::Checkbox("Show deviation colors", &material.showDeviation);
    ImGui::EndDisabled();
    if (entry.result() && material.showDeviation)
    {
        static const char *maps[] = {"Jet", "Turbo", "Viridis", "Hot", "Cool", "Gray"};
        int map = static_cast<int>(material.colorMap);
        changed |= ImGui::Combo("Color map", &map, maps, IM_ARRAYSIZE(maps));
        material.colorMap = static_cast<ColorMap>(map);
        static const char *methods[] = {"Median edge", "Mean edge"};
        int method = static_cast<int>(material.sigmaMethod);
        changed |= ImGui::Combo("Normalize by", &method, methods, IM_ARRAYSIZE(methods));
        material.sigmaMethod = static_cast<SigmaMethod>(method);
        changed |= ImGui::DragFloat("Sigma scale", &material.sigmaScale, 0.02f, 0.001f, 1000.0f, "%.3f");
    }
    else
    {
        float color[] = {material.baseColor.x, material.baseColor.y, material.baseColor.z};
        changed |= ImGui::ColorEdit3("Base color", color);
        material.baseColor = make_float3(color[0], color[1], color[2]);
    }
    if (changed)
    {
        try { m_workspace.setMaterial(entry.id, material); m_status.clear(); }
        catch (const std::exception &e) { m_status = e.what(); }
    }
    if (entry.result() && entry.material.showDeviation)
    {
        const auto samples = colorMapSamples(entry.material.colorMap);
        const auto pos = ImGui::GetCursorScreenPos();
        const float width = std::max(1.0f, ImGui::GetContentRegionAvail().x);
        auto *draw = ImGui::GetWindowDrawList();
        for (size_t i = 0; i+1 < samples.size(); ++i)
        {
            auto rgba = [](float3 c) { return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x,c.y,c.z,1)); };
            const auto a = rgba(samples[i]), b = rgba(samples[i+1]);
            draw->AddRectFilledMultiColor(ImVec2(pos.x+width*i/(samples.size()-1),pos.y),
                ImVec2(pos.x+width*(i+1)/(samples.size()-1),pos.y+18), a,b,b,a);
        }
        ImGui::Dummy(ImVec2(width, 22));
        ImGui::Text("Range: 0 to %.6g (clamped)", entry.sigma());
    }
    ImGui::Separator();
    auto [pathIt, inserted] = m_outputPaths.try_emplace(entry.id);
    auto &path = pathIt->second;
    if (inserted)
    {
        auto suggested = entry.lastExport;
        if (suggested.empty())
        {
            suggested = entry.inputPath.empty() ? std::filesystem::path("mesh.obj") : entry.inputPath;
            suggested.replace_filename(suggested.stem().u8string()+"_deviation_"+std::to_string(entry.id)+".obj");
        }
        std::snprintf(path.data(), path.size(), "%s", suggested.u8string().c_str());
    }
    ImGui::TextUnformatted("Export selected mesh with current colors");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##outputPath", path.data(), path.size());
    if (ImGui::Button("Save OBJ"))
    {
        try { m_status = "Saved: " + m_workspace.save(entry.id, std::filesystem::u8path(path.data())).u8string(); }
        catch (const std::exception &e) { m_status = std::string("Save failed: ") + e.what(); }
    }
    ImGui::SameLine();
    ImGui::TextUnformatted(entry.savedRevision == entry.revision ? "Saved" : "Not saved");
    ImGui::TextWrapped("%s", m_status.c_str());
}
void MaterialPanel::draw()
{
    if (ImGui::Begin("Material"))
    {
        const auto *entry = m_workspace.selected();
        const auto id = entry ? entry->id : NoMesh;
        if (id != m_selected) { m_selected = id; m_status.clear(); }
        if (entry) drawMaterial(*entry);
        else ImGui::TextWrapped("Select a mesh in Mesh List to edit its material and save.");
    }
    ImGui::End();
}
}
