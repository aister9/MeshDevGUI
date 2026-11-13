#pragma once
#include "Visualizer/UI/Components.h"
#include "workspace/MeshWorkspace.h"

namespace MeshDev
{
class DeviationPanel final : public ImGUIAX::Visualizer::Components
{
public:
    explicit DeviationPanel(MeshWorkspace &workspace) : m_workspace(workspace) {}
    void draw() override;
private:
    MeshWorkspace &m_workspace;
    int m_mode = 1;
    std::string m_status;
};
}
