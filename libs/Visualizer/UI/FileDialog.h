#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "Components.h"

namespace SPIN
{
    namespace Visualizer
    {
        class FileDialogHierarchyView final : public Components
        {
        public:
            using DirectorySelectedCallback = std::function<void(const std::filesystem::path &)>;

            FileDialogHierarchyView() = default;

            void refreshSystemRoots();
            void setRoot(const std::filesystem::path &path);
            void setSelectedDirectory(const std::filesystem::path &path);
            const std::filesystem::path &selectedDirectory() const;
            void onDirectorySelected(DirectorySelectedCallback callback);

            void draw() override;

        private:
            std::vector<std::filesystem::path> m_rootEntries;
            std::filesystem::path m_userRoot;
            std::filesystem::path m_selectedDirectory;
            DirectorySelectedCallback m_onDirectorySelected;

            void drawDirectoryRecursive(const std::filesystem::path &path);
            void ensureRootEntry(const std::filesystem::path &path);
        };

        class FileDialogFileSelectView final : public Components
        {
        public:
            using DirectoryChangeCallback = std::function<void(const std::filesystem::path &)>;
            using FileSelectedCallback = std::function<void(const std::filesystem::path &)>;
            using FileActivatedCallback = std::function<void(const std::filesystem::path &)>;

            FileDialogFileSelectView() = default;

            void setCurrentDirectory(const std::filesystem::path &path);
            const std::filesystem::path &currentDirectory() const;

            void setExtensionFilter(const std::filesystem::path &ext);
            const std::filesystem::path &extensionFilter() const;

            void setMultiSelectable(bool enable);
            bool isMultiSelectable() const;

            void onDirectoryChange(DirectoryChangeCallback callback);
            void onFileSelected(FileSelectedCallback callback);
            void onFileActivated(FileActivatedCallback callback);

            const std::filesystem::path &getSelectedFile() const;
            const std::vector<std::filesystem::path> &getSelectedFiles() const;

            void draw() override;

        private:
            bool m_multiSelectable = false;
            std::filesystem::path m_currentDir;
            std::filesystem::path m_extensionFilter;
            std::filesystem::path m_selectedFile;
            std::vector<std::filesystem::path> m_selectedFiles;

            DirectoryChangeCallback m_directoryChange;
            FileSelectedCallback m_fileSelected;
            FileActivatedCallback m_fileActivated;
        };

        class FileDialogTopbar final : public Components
        {
        public:
            using NavigationCallback = std::function<void()>;
            using PathCommitCallback = std::function<void(const std::filesystem::path &)>;

            FileDialogTopbar() = default;

            void setCurrentPath(const std::filesystem::path &path);
            void setNavigationCallbacks(NavigationCallback onBack,
                                        NavigationCallback onForward,
                                        NavigationCallback onUp,
                                        NavigationCallback onRefresh);
            void onPathCommit(PathCommitCallback callback);

            void draw() override;

        private:
            std::filesystem::path m_currentPath;
            std::array<char, 512> m_pathBuffer{};

            NavigationCallback m_onBack;
            NavigationCallback m_onForward;
            NavigationCallback m_onUp;
            NavigationCallback m_onRefresh;
            PathCommitCallback m_onPathCommit;

            void syncBuffer();
        };

        class FileDialogBottombar final : public Components
        {
        public:
            FileDialogBottombar() = default;

            void setDirectory(const std::filesystem::path &path);
            void setFilename(const std::string &filename);
            void setSelection(const std::vector<std::filesystem::path> &selection);
            bool consumeConfirmed(std::string &outFilename);
            bool consumeCancelled();

            void draw() override;

        private:
            std::filesystem::path m_currentDirectory;
            std::string m_filenameInput;
            bool m_confirmed = false;
            bool m_cancelled = false;
            std::array<char, 260> m_filenameBuffer{};
            std::vector<std::filesystem::path> m_selectedFiles;

            void syncBuffer();
        };

        class FileDialog final : public Components
        {
        public:
            FileDialog();

            void setRootDirectory(const std::filesystem::path &path);
            void setExtensionFilter(const std::filesystem::path &ext);
            void enableMultiSelect(bool enable);

            void openDialog();
            void setVisible(bool visible);
            bool isVisible() const;

            void draw() override;

            // Returns confirmed file path; empty if not confirmed yet.
            std::filesystem::path open() const;
            const std::vector<std::filesystem::path> &openMultiple() const;

        private:
            FileDialogHierarchyView m_hierarchyView;
            FileDialogFileSelectView m_fileSelectView;
            FileDialogTopbar m_topbar;
            FileDialogBottombar m_bottombar;

            std::filesystem::path m_rootDirectory;
            std::filesystem::path m_currentDirectory;
            std::filesystem::path m_selectedFile;
            std::filesystem::path m_lastConfirmedPath;
            std::vector<std::filesystem::path> m_lastConfirmedList;
            std::vector<std::filesystem::path> m_history;
            int m_historyCursor = -1;
            bool m_isVisible = false;
            bool m_multiSelectEnabled = false;

            void setCurrentDirectory(const std::filesystem::path &path, bool pushHistory = true);
            void navigateBack();
            void navigateForward();
            void navigateUp();
            void refresh();
            void connectCallbacks();
            void confirmSelection(const std::filesystem::path &path);
            void confirmSelection(std::vector<std::filesystem::path> paths);
        };
    }
}
