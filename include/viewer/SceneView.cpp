#include <GL/glew.h>
#include "viewer/SceneView.h"
#include "viewer/FrameImage.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace MeshDev
{
SceneView::~SceneView()
{
    if (m_texture)
        glDeleteTextures(1, &m_texture);
}

bool SceneView::showMesh(const TriangleMesh &mesh, const std::vector<float3> &colors, const std::string &label)
{
    try
    {
        if (!m_renderer)
            m_renderer = createOwlRenderer();
        float3 minimum = make_float3(INFINITY), maximum = make_float3(-INFINITY);
        for (const auto &v : mesh.vertex)
        {
            minimum = fminf(minimum,v);
            maximum = fmaxf(maximum,v);
        }
        m_renderer->setMesh(mesh, colors);
        m_minimum = minimum;
        m_maximum = maximum;
        m_label = label;
        m_vertices = mesh.vertex.size();
        m_triangles = mesh.index.size();
        m_hasMesh = true;
        m_fitPending = !m_cameraInitialized;
        m_dirty = true;
        m_error.clear();
        return true;
    }
    catch (const std::exception &e)
    {
        m_error = e.what();
        return false;
    }
}

void SceneView::synchronizeSelection()
{
    const auto *entry = m_workspace.selected();
    const auto id = entry ? entry->id : NoMesh;
    if (id != m_displayed)
    {
        // A failed selection must not leave another mesh visible under its name.
        m_renderer.reset();
        m_hasMesh = false;
        m_error.clear();
        m_displayed = id;
        m_revision = 0;
    }
    if (!entry || m_revision == entry->revision) return;
    m_revision = entry->revision;
    try
    {
        const auto colors = m_workspace.colors(id);
        if (!m_hasMesh) showMesh(entry->mesh, colors, entry->name);
        else
        {
            m_renderer->setColors(colors);
            m_dirty = true;
            m_error.clear();
        }
    }
    catch (const std::exception &e) { m_error = e.what(); m_hasMesh = false; }
}

void SceneView::fitCamera()
{
    if (m_hasMesh)
    {
        m_camera.fit(m_minimum, m_maximum, float(m_width)/m_height);
        m_cameraInitialized = true;
        m_fitPending = false;
        m_dirty = true;
    }
}

void SceneView::draw()
{
    m_visible = ImGui::Begin("Scene View");
    if (m_visible)
    {
        synchronizeSelection();
        if (ImGui::Button("Fit")) fitCamera();
        ImGui::SameLine();
        if (ImGui::Checkbox("Lighting", &m_lighting)) m_dirty = true;
        ImGui::SameLine();
        drawImageSave();
        ImGui::TextUnformatted("Left drag: orbit | Right/middle drag: pan | Wheel: zoom");
        if (!m_error.empty())
        {
            ImGui::TextWrapped("Render error: %s", m_error.c_str());
            if (ImGui::Button("Retry render")) { m_revision = 0; m_dirty = true; }
        }
        if (!m_hasMesh)
            ImGui::TextUnformatted("Select a mesh in Mesh List to display it here.");
        else
        {
            ImGui::Text("%s | %zu vertices, %zu triangles", m_label.c_str(),m_vertices,m_triangles);
            const ImVec2 available = ImGui::GetContentRegionAvail();
            if (available.x >= 1 && available.y >= 1)
            {
                // Cap the framebuffer while preserving aspect ratio on large/high-DPI displays.
                const float scale = std::min(1.0f, 2048.0f/std::max(available.x,available.y));
                const int width = std::max(1, int(available.x*scale));
                const int height = std::max(1, int(available.y*scale));
                if (width != m_width || height != m_height)
                {
                    m_width = width;
                    m_height = height;
                    m_dirty = true;
                }
                if (m_fitPending) fitCamera();
                if (!m_texture)
                {
                    glGenTextures(1,&m_texture);
                    GLint previous = 0;
                    glGetIntegerv(GL_TEXTURE_BINDING_2D,&previous);
                    glBindTexture(GL_TEXTURE_2D,m_texture);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
                    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
                    const std::uint32_t background = 0xff16110e;
                    glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,1,1,0,GL_RGBA,GL_UNSIGNED_BYTE,&background);
                    glBindTexture(GL_TEXTURE_2D,previous);
                }
                ImGui::Image((ImTextureID)(intptr_t)m_texture,available);
                // Claim mouse drags so they don't move the ImGui window.
                ImGui::SetCursorScreenPos(ImGui::GetItemRectMin());
                ImGui::InvisibleButton("##camera",available,
                    ImGuiButtonFlags_MouseButtonLeft|ImGuiButtonFlags_MouseButtonRight|ImGuiButtonFlags_MouseButtonMiddle);
                const ImGuiIO &io = ImGui::GetIO();
                if (ImGui::IsItemHovered() && io.MouseWheel != 0)
                {
                    m_camera.zoom(io.MouseWheel);
                    m_dirty = true;
                }
                if (ImGui::IsItemActive())
                {
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
                    {
                        m_camera.orbit(io.MouseDelta.x,io.MouseDelta.y);
                        m_dirty = true;
                    }
                    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle))
                    {
                        m_camera.pan(io.MouseDelta.x/available.x,io.MouseDelta.y/available.y,float(m_width)/m_height);
                        m_dirty = true;
                    }
                }
            }
            else m_visible = false;
        }
    }
    ImGui::End();
}

void SceneView::render()
{
    if (!m_visible || !m_hasMesh || !m_dirty || !m_texture)
        return;
    m_dirty = false;
    try
    {
        const auto frame = m_renderer->render(m_camera.frame(float(m_width)/m_height),m_width,m_height,m_lighting);
        GLint previousTexture = 0, previousBuffer = 0, previousAlignment = 0, previousRowLength = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&previousTexture);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING,&previousBuffer);
        glGetIntegerv(GL_UNPACK_ALIGNMENT,&previousAlignment);
        glGetIntegerv(GL_UNPACK_ROW_LENGTH,&previousRowLength);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
        glPixelStorei(GL_UNPACK_ALIGNMENT,4);
        glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
        glBindTexture(GL_TEXTURE_2D,m_texture);
        if (frame.width != m_textureWidth || frame.height != m_textureHeight)
        {
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,frame.width,frame.height,0,GL_RGBA,GL_UNSIGNED_BYTE,frame.rgba);
            m_textureWidth = frame.width;
            m_textureHeight = frame.height;
        }
        else glTexSubImage2D(GL_TEXTURE_2D,0,0,0,frame.width,frame.height,GL_RGBA,GL_UNSIGNED_BYTE,frame.rgba);
        glBindTexture(GL_TEXTURE_2D,previousTexture);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER,previousBuffer);
        glPixelStorei(GL_UNPACK_ALIGNMENT,previousAlignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH,previousRowLength);
        m_error.clear();
        if (!m_pendingImageDirectory.empty())
        {
            try
            {
                const auto *entry = m_workspace.selected();
                if (!entry || entry->id != m_pendingImageMesh)
                    throw std::runtime_error("Selected mesh changed before image export");
                const auto path = saveNumberedFramePng(frame, m_pendingImageDirectory, *entry);
                m_imageStatus = "Saved: " + path.u8string();
            }
            catch (const std::exception &e) { m_imageStatus = std::string("Save failed: ") + e.what(); }
            m_pendingImageDirectory.clear();
        }
    }
    catch (const std::exception &e)
    {
        m_error = e.what();
        if (!m_pendingImageDirectory.empty())
        {
            m_imageStatus = "Save failed: rendering did not complete.";
            m_pendingImageDirectory.clear();
        }
    }
}

void SceneView::drawImageSave()
{
    const auto *entry = m_workspace.selected();
    ImGui::BeginDisabled(!m_hasMesh || !entry);
    if (ImGui::Button("Save PNG..."))
    {
        try
        {
            auto initial = m_imageDirectory;
            if (initial.empty() || !std::filesystem::is_directory(initial))
                initial = entry->inputPath.empty() ? std::filesystem::current_path() : entry->inputPath.parent_path();
            m_imageDialog.open(initial);
        }
        catch (const std::exception &e) { m_imageStatus = std::string("Save failed: ") + e.what(); }
    }
    ImGui::SameLine();
    if (ImGui::Button("Quick Save PNG"))
    {
        try { queueImageSave(quickImageDirectory(*entry)); }
        catch (const std::exception &e) { m_imageStatus = std::string("Save failed: ") + e.what(); }
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Save to the source model's output folder with the next image number.");
    ImGui::EndDisabled();
    if (entry)
    {
        if (const auto directory = m_imageDialog.draw(*entry))
        {
            m_imageDirectory = *directory;
            queueImageSave(*directory);
        }
    }
    // Reserve one line so saving does not resize the viewport or change its aspect ratio.
    ImGui::TextUnformatted(m_imageStatus.empty() ? " " : m_imageStatus.c_str());
    if (!m_imageStatus.empty() && ImGui::IsItemHovered())
        ImGui::SetTooltip("%s", m_imageStatus.c_str());
}

void SceneView::queueImageSave(const std::filesystem::path &directory)
{
    m_pendingImageDirectory = directory;
    m_pendingImageMesh = m_displayed;
    m_dirty = true; // Render the final camera/material state before writing.
    m_imageStatus = "Saving PNG...";
}
}
