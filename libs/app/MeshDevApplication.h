#pragma once
#include "Visualizer/Application.h"
#include "workspace/MeshWorkspace.h"

namespace MeshDev
{
class MeshDevApplication final : public ImGUIAX::Visualizer::Application
{
public:
    const char *title() const override { return "MeshDev GUI"; }
    void start(ImGUIAX::Visualizer::Stage &stage) override;
private:
    MeshWorkspace m_workspace;
};
}
