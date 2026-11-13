#include "viewer/DeviationPanel.h"
#include <exception>

namespace MeshDev
{
void DeviationPanel::draw()
{
    if (ImGui::Begin("Deviation"))
    {
        const auto *source = m_workspace.find(m_workspace.source());
        const auto *target = m_workspace.find(m_workspace.target());
        ImGui::TextWrapped("Source: %s", source ? source->name.c_str() : "Choose in Mesh List");
        ImGui::TextWrapped("Target: %s", target ? target->name.c_str() : "Choose in Mesh List");
        ImGui::TextWrapped("Distance from each Target vertex to the Source surface.");
        static const char *modes[] = {"CPU", "GPU", "CPU + GPU"};
        ImGui::SetNextItemWidth(-1);
        ImGui::Combo("##computeMode", &m_mode, modes, IM_ARRAYSIZE(modes));
        ImGui::BeginDisabled(!m_workspace.canCompute());
        if (ImGui::Button("Calculate Deviation"))
        {
            try
            {
                m_workspace.compute(static_cast<ComputeMode>(m_mode));
                m_status = "Calculation complete. Adjust colors in Material, then Save to export.";
            }
            catch (const std::exception &e) { m_status = std::string("Calculation failed: ") + e.what(); }
        }
        ImGui::EndDisabled();
        ImGui::TextWrapped("%s", m_status.c_str());
    }
    ImGui::End();
}
}
