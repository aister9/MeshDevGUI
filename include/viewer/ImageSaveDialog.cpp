#include "viewer/ImageSaveDialog.h"
#include "viewer/FrameImage.h"
#include <cstdio>
#include <stdexcept>

namespace MeshDev
{
ImageSaveDialog::ImageSaveDialog()
{
    m_hierarchy.onDirectorySelected([this](const auto &path) { navigate(path); });
    m_files.setExtensionFilter(".png");
    m_files.onDirectoryChange([this](const auto &path) { navigate(path); });
}

void ImageSaveDialog::navigate(const std::filesystem::path &directory)
{
    try
    {
        const auto path = std::filesystem::absolute(directory).lexically_normal();
        if (!std::filesystem::is_directory(path)) throw std::runtime_error("Folder does not exist");
        m_directory = path;
        m_hierarchy.setSelectedDirectory(path);
        m_files.setCurrentDirectory(path);
        std::snprintf(m_path.data(), m_path.size(), "%s", path.u8string().c_str());
        m_error.clear();
    }
    catch (const std::exception &e) { m_error = e.what(); }
}

void ImageSaveDialog::open(const std::filesystem::path &initialDirectory)
{
    m_directory.clear();
    m_path.fill(0);
    m_error.clear();
    m_hierarchy.refreshSystemRoots();
    navigate(initialDirectory);
    if (!m_directory.empty()) m_hierarchy.setRoot(m_directory);
    ImGui::OpenPopup("Save scene image");
}

std::optional<std::filesystem::path> ImageSaveDialog::draw(const MeshEntry &entry)
{
    std::optional<std::filesystem::path> selected;
    ImGui::SetNextWindowSize(ImVec2(620, 460), ImGuiCond_FirstUseEver);
    if (ImGui::BeginPopupModal("Save scene image", nullptr))
    {
        ImGui::TextUnformatted("Choose the folder for this PNG image.");
        ImGui::SetNextItemWidth(-100);
        const bool enter = ImGui::InputText("##folder", m_path.data(), m_path.size(), ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        if (ImGui::Button("Go") || enter) navigate(std::filesystem::u8path(m_path.data()));
        ImGui::SameLine();
        if (ImGui::Button("Up") && !m_directory.empty()) navigate(m_directory.parent_path());
        ImGui::BeginChild("Folders", ImVec2(180, -120), true);
        m_hierarchy.draw();
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginChild("Folder contents", ImVec2(0, -120), true);
        m_files.draw();
        ImGui::EndChild();
        std::filesystem::path output;
        try
        {
            if (!m_directory.empty()) output = nextFrameImagePath(m_directory, entry);
        }
        catch (const std::exception &e) { m_error = e.what(); }
        ImGui::Text("File: %s", output.filename().u8string().c_str());
        ImGui::TextUnformatted(m_directory.u8string().c_str());
        const bool pathEdited = m_directory.u8string() != m_path.data();
        ImGui::TextUnformatted(!m_error.empty() ? m_error.c_str() : pathEdited ? "Press Go to select the entered folder." : " ");
        ImGui::BeginDisabled(output.empty() || !m_error.empty() || pathEdited);
        if (ImGui::Button("Save here"))
        {
            selected = m_directory;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    return selected;
}
}
