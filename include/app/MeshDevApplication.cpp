#include "app/MeshDevApplication.h"
#include "viewer/MeshListPanel.h"
#include "viewer/DeviationPanel.h"
#include "viewer/MaterialPanel.h"
#include "viewer/SceneView.h"
#include <imgui_internal.h>

namespace MeshDev
{
namespace
{
class WorkspaceLayout final : public ImGUIAX::Visualizer::Components
{
public:
    void draw() override
    {
        const auto *viewport = ImGui::GetMainViewport();
        const auto id = ImGui::GetID("MeshDevWorkspace");
        if (!ImGui::DockBuilderGetNode(id))
        {
            ImGui::DockBuilderAddNode(id, ImGuiDockNodeFlags_DockSpace);
            ImGui::DockBuilderSetNodeSize(id, viewport->WorkSize);
            auto center = id;
            const auto left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.23f, nullptr, &center);
            const auto right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.30f, nullptr, &center);
            auto list = left;
            const auto deviation = ImGui::DockBuilderSplitNode(list, ImGuiDir_Down, 0.34f, nullptr, &list);
            ImGui::DockBuilderDockWindow("Mesh List", list);
            ImGui::DockBuilderDockWindow("Deviation", deviation);
            ImGui::DockBuilderDockWindow("Material", right);
            ImGui::DockBuilderDockWindow("Scene View", center);
            ImGui::DockBuilderFinish(id);
        }
        ImGui::DockSpaceOverViewport(id, viewport);
    }
};
}
void MeshDevApplication::start(ImGUIAX::Visualizer::Stage &stage)
{
    stage.setClearColor(initialClearColor());
    stage.emplaceDrawable<WorkspaceLayout>();
    stage.emplaceDrawable<MeshListPanel>(m_workspace);
    stage.emplaceDrawable<DeviationPanel>(m_workspace);
    stage.emplaceDrawable<MaterialPanel>(m_workspace);
    auto &scene = stage.emplaceDrawable<SceneView>(m_workspace);
    stage.setSceneRenderer([&scene](ImGUIAX::Visualizer::Stage &, float) { scene.render(); });
}
}
