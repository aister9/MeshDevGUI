#pragma once
#include "Visualizer/UI/Components.h"
#include "workspace/MeshWorkspace.h"
#include <array>
#include <unordered_map>

namespace MeshDev
{
class MaterialPanel final : public ImGUIAX::Visualizer::Components
{
public:
    explicit MaterialPanel(MeshWorkspace &workspace) : m_workspace(workspace) {}
    void draw() override;
private:
    void drawMaterial(const MeshEntry &entry);
    MeshWorkspace &m_workspace;
    std::unordered_map<MeshId, std::array<char, 1024>> m_outputPaths;
    MeshId m_selected = NoMesh;
    std::string m_status;
};
}
