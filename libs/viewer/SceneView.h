#pragma once

#include "Visualizer/UI/Components.h"
#include "render/MeshRenderer.h"
#include "workspace/MeshWorkspace.h"
#include "viewer/ImageSaveDialog.h"
#include <string>

namespace MeshDev
{
// Stage owns the component and destroys its GL texture before destroying the GL context.
class SceneView final : public ImGUIAX::Visualizer::Components
{
public:
    explicit SceneView(const MeshWorkspace &workspace) : m_workspace(workspace) {}
    ~SceneView() override;
    void draw() override;
    void render(); // Called from Stage::SceneRenderer, before ImGui submits its draw data.

private:
    bool showMesh(const TriangleMesh &mesh, const std::vector<float3> &colors, const std::string &label);
    void synchronizeSelection();
    void fitCamera();
    void drawImageSave();
    void queueImageSave(const std::filesystem::path &directory);
    const MeshWorkspace &m_workspace;
    MeshId m_displayed = NoMesh;
    std::uint64_t m_revision = 0;
    bool m_fitPending = false;
    bool m_cameraInitialized = false;
    std::unique_ptr<MeshRenderer> m_renderer;
    OrbitCamera m_camera;
    float3 m_minimum{}, m_maximum{};
    unsigned int m_texture = 0;
    int m_width = 1, m_height = 1;
    int m_textureWidth = 0, m_textureHeight = 0;
    bool m_visible = false;
    bool m_dirty = false;
    bool m_hasMesh = false;
    bool m_lighting = false;
    std::string m_label;
    std::string m_error;
    ImageSaveDialog m_imageDialog;
    std::filesystem::path m_imageDirectory;
    std::filesystem::path m_pendingImageDirectory;
    MeshId m_pendingImageMesh = NoMesh;
    std::string m_imageStatus;
    size_t m_vertices = 0, m_triangles = 0;
};
}
