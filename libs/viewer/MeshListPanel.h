#pragma once
#include "Visualizer/UI/FileDialog.h"
#include "workspace/MeshWorkspace.h"
#include <array>

namespace MeshDev
{
class MeshListPanel final : public ImGUIAX::Visualizer::Components
{
public:
    explicit MeshListPanel(MeshWorkspace &workspace) : m_workspace(workspace) {}
    void draw() override;
private:
    void loadFiles(const std::vector<std::filesystem::path> &paths);
    MeshWorkspace &m_workspace;
    std::unique_ptr<ImGUIAX::Visualizer::FileDialog> m_dialog;
    std::array<char, 1024> m_path{};
    std::string m_status;
};
}
