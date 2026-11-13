#pragma once

#include "Visualizer/UI/FileDialog.h"
#include "workspace/MeshWorkspace.h"
#include <optional>

namespace MeshDev
{
// Folder selection for numbered image exports; reuses the existing ImGUIAX browser.
class ImageSaveDialog
{
public:
    ImageSaveDialog();
    void open(const std::filesystem::path &initialDirectory);
    std::optional<std::filesystem::path> draw(const MeshEntry &entry);
private:
    void navigate(const std::filesystem::path &directory);
    ImGUIAX::Visualizer::FileDialogHierarchyView m_hierarchy;
    ImGUIAX::Visualizer::FileDialogFileSelectView m_files;
    std::filesystem::path m_directory;
    std::array<char, 2048> m_path{};
    std::string m_error;
};
}
