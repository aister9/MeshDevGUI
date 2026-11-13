#include "viewer/MeshListPanel.h"
#include <exception>
#include <algorithm>

namespace MeshDev
{
void MeshListPanel::loadFiles(const std::vector<std::filesystem::path> &paths)
{
    size_t count = 0;
    std::string errors;
    for (const auto &path : paths)
    {
        try { count += m_workspace.load(path).size(); }
        catch (const std::exception &e) { errors += std::string("\n") + e.what(); }
    }
    m_status = "Loaded " + std::to_string(count) + " mesh(es)." + errors;
}
void MeshListPanel::draw()
{
    if (ImGui::Begin("Mesh List"))
    {
        if (ImGui::Button("Open OBJ files..."))
        {
            // A fresh dialog cannot reuse a previous import confirmation.
            m_dialog = std::make_unique<ImGUIAX::Visualizer::FileDialog>();
            m_dialog->setExtensionFilter(".obj");
            m_dialog->enableMultiSelect(true);
            // ImGUIAX prepends the current directory to relative selections.
            // Its directory entries must therefore start from an absolute root.
            if (std::filesystem::is_directory("../../dataset"))
                m_dialog->setRootDirectory(std::filesystem::canonical("../../dataset"));
            m_dialog->openDialog();
        }
        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##inputPath", "OBJ path", m_path.data(), m_path.size());
        ImGui::BeginDisabled(m_path[0] == '\0');
        if (ImGui::Button("Add path")) loadFiles({std::filesystem::u8path(m_path.data())});
        ImGui::EndDisabled();
        ImGui::TextWrapped("Select a row to display it in Scene View.");
        const auto *selected = m_workspace.selected();
        ImGui::BeginDisabled(!selected);
        if (ImGui::Button("Set Source") && selected) m_workspace.setSource(selected->id);
        ImGui::SameLine();
        if (ImGui::Button("Set Target") && selected) m_workspace.setTarget(selected->id);
        if (ImGui::Button("Remove from list") && selected) m_workspace.remove(selected->id);
        ImGui::EndDisabled();
        if (ImGui::BeginTable("meshes", 3, ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_RowBg
            | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable,
            ImVec2(0, std::max(90.0f, ImGui::GetContentRegionAvail().y-65))))
        {
            ImGui::TableSetupColumn("Mesh", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Role", ImGuiTableColumnFlags_WidthFixed, 52);
            ImGui::TableSetupColumn("Result", ImGuiTableColumnFlags_WidthFixed, 48);
            ImGui::TableHeadersRow();
            for (const auto &entry : m_workspace.entries())
            {
                ImGui::PushID(entry.get());
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                if (ImGui::Selectable(entry->name.c_str(), m_workspace.selected() == entry.get(), ImGuiSelectableFlags_SpanAllColumns))
                    m_workspace.select(entry->id);
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextUnformatted(entry->inputPath.u8string().c_str());
                    ImGui::Text("%zu vertices / %zu triangles", entry->mesh.vertex.size(), entry->mesh.index.size());
                    ImGui::EndTooltip();
                }
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(entry->id == m_workspace.source() ? "Source" :
                                       entry->id == m_workspace.target() ? "Target" : "");
                ImGui::TableNextColumn();
                if (entry->result()) ImGui::TextUnformatted(entry->result()->backend.c_str());
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::TextWrapped("%s", m_status.c_str());
    }
    ImGui::End();
    if (m_dialog && m_dialog->isVisible())
    {
        m_dialog->draw();
        if (!m_dialog->isVisible() && !m_dialog->openMultiple().empty()) loadFiles(m_dialog->openMultiple());
    }
}
}
