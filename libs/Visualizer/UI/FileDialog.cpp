#include "FileDialog.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <system_error>

#include <imgui.h>

namespace
{
    constexpr const char *ICON_FOLDER = "[DIR]";
    constexpr const char *ICON_FILE = "FILE";
    constexpr const char *ICON_BACK = "<";
    constexpr const char *ICON_FORWARD = ">";
    constexpr const char *ICON_UP = "^";
    constexpr const char *ICON_REFRESH = "R";

    std::string PathToUTF8(const std::filesystem::path &path)
    {
#if defined(_WIN32)
        return path.u8string();
#else
        return path.string();
#endif
    }

    std::string MakeTreeLabel(const std::filesystem::path &path)
    {
        std::string displayName = PathToUTF8(path.filename());
        if (displayName.empty())
        {
            displayName = PathToUTF8(path);
        }
        std::string label = ICON_FOLDER;
        label.push_back(' ');
        label += displayName;
        label += "##";
        label += PathToUTF8(path);
        return label;
    }

    std::string FormatFileSize(uintmax_t size)
    {
        constexpr const char *units[] = {"B", "KB", "MB", "GB", "TB"};
        double value = static_cast<double>(size);
        size_t unitIdx = 0U;
        while (value >= 1024.0 && unitIdx < std::size(units) - 1)
        {
            value /= 1024.0;
            ++unitIdx;
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(unitIdx == 0 ? 0 : 1) << value << ' ' << units[unitIdx];
        return oss.str();
    }

    std::string FormatTime(const std::filesystem::file_time_type &time)
    {
        using namespace std::chrono;
        auto sctp = time_point_cast<system_clock::duration>(time - std::filesystem::file_time_type::clock::now()
                                                            + system_clock::now());
        std::time_t tt = system_clock::to_time_t(sctp);
        std::tm tmBuf{};
#if defined(_WIN32)
        localtime_s(&tmBuf, &tt);
#else
        localtime_r(&tt, &tmBuf);
#endif
        std::ostringstream oss;
        oss << std::put_time(&tmBuf, "%Y-%m-%d %H:%M");
        return oss.str();
    }
}

namespace SPIN
{
    namespace Visualizer
    {
        // ----- FileDialogHierarchyView --------------------------------------------------------
        void FileDialogHierarchyView::refreshSystemRoots()
        {
            m_rootEntries.clear();
            std::error_code ec;

#if defined(_WIN32)
            for (char letter = 'A'; letter <= 'Z'; ++letter)
            {
                std::string drive;
                drive += letter;
                drive += ":/";
                std::filesystem::path root(drive);
                if (std::filesystem::exists(root, ec))
                {
                    m_rootEntries.push_back(root);
                }
            }
#else
            m_rootEntries.emplace_back(std::filesystem::path("/"));
#endif

            const char *homeEnv =
#if defined(_WIN32)
                std::getenv("USERPROFILE");
#else
                std::getenv("HOME");
#endif
            if (homeEnv)
            {
                std::filesystem::path homePath(homeEnv);
                if (std::filesystem::exists(homePath, ec) && std::filesystem::is_directory(homePath, ec))
                {
                    ensureRootEntry(homePath);
                }
            }

            if (!m_userRoot.empty())
            {
                ensureRootEntry(m_userRoot);
            }
        }

        void FileDialogHierarchyView::setRoot(const std::filesystem::path &path)
        {
            m_userRoot = path;
            ensureRootEntry(path);
            m_selectedDirectory = m_userRoot;
        }

        void FileDialogHierarchyView::setSelectedDirectory(const std::filesystem::path &path)
        {
            if (path.empty())
            {
                return;
            }
            ensureRootEntry(path);
            m_selectedDirectory = path;
        }

        const std::filesystem::path &FileDialogHierarchyView::selectedDirectory() const
        {
            return m_selectedDirectory;
        }

        void FileDialogHierarchyView::onDirectorySelected(DirectorySelectedCallback callback)
        {
            m_onDirectorySelected = std::move(callback);
        }

        void FileDialogHierarchyView::draw()
        {
            if (m_rootEntries.empty())
            {
                refreshSystemRoots();
            }

            ImGui::TextUnformatted("Directories");
            ImGui::Separator();
            for (const auto &root : m_rootEntries)
            {
                drawDirectoryRecursive(root);
            }
        }

        void FileDialogHierarchyView::drawDirectoryRecursive(const std::filesystem::path &path)
        {
            std::error_code ec;
            std::vector<std::filesystem::path> children;
            bool enumerationFailed = false;
            for (std::filesystem::directory_iterator it(path, ec); !ec && it != std::filesystem::directory_iterator(); ++it)
            {
                std::error_code dirEc;
                if (it->is_directory(dirEc))
                {
                    children.push_back(it->path());
                }
            }
            if (ec)
            {
                enumerationFailed = true;
                ec.clear();
            }
            std::sort(children.begin(), children.end(), [](const auto &a, const auto &b) {
                return PathToUTF8(a.filename()) < PathToUTF8(b.filename());
            });

            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanFullWidth;
            if (children.empty())
            {
                flags |= ImGuiTreeNodeFlags_Leaf;
            }
            if (path == m_selectedDirectory)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            const std::string label = MakeTreeLabel(path);
            bool open = ImGui::TreeNodeEx(label.c_str(), flags);
            if (ImGui::IsItemClicked())
            {
                m_selectedDirectory = path;
                if (m_onDirectorySelected)
                {
                    m_onDirectorySelected(path);
                }
            }

            if (open)
            {
                if (enumerationFailed)
                {
                    const std::string pathUtf8 = PathToUTF8(path);
                    ImGui::TextDisabled("Unable to list: %s", pathUtf8.c_str());
                }
                else
                {
                    for (const auto &child : children)
                    {
                        drawDirectoryRecursive(child);
                    }
                }
                ImGui::TreePop();
            }
        }

        void FileDialogHierarchyView::ensureRootEntry(const std::filesystem::path &path)
        {
            if (path.empty())
            {
                return;
            }

            std::filesystem::path candidate = path;
            if (candidate.has_root_path())
            {
                candidate = candidate.root_path();
            }

            const std::string normalizedCandidate = candidate.lexically_normal().generic_u8string();
            auto it = std::find_if(m_rootEntries.begin(), m_rootEntries.end(),
                                   [&](const std::filesystem::path &root) {
                                       return root.lexically_normal().generic_u8string() == normalizedCandidate;
                                   });

            if (it == m_rootEntries.end())
            {
                std::error_code ec;
                if (std::filesystem::exists(candidate, ec))
                {
                    m_rootEntries.push_back(candidate);
                }
            }
        }

        // ----- FileDialogFileSelectView -------------------------------------------------------
        void FileDialogFileSelectView::setCurrentDirectory(const std::filesystem::path &path)
        {
            m_currentDir = path;
            m_selectedFile.clear();
            m_selectedFiles.clear();
        }

        const std::filesystem::path &FileDialogFileSelectView::currentDirectory() const
        {
            return m_currentDir;
        }

        void FileDialogFileSelectView::setExtensionFilter(const std::filesystem::path &ext)
        {
            m_extensionFilter = ext;
            m_selectedFile.clear();
            m_selectedFiles.clear();
        }

        const std::filesystem::path &FileDialogFileSelectView::extensionFilter() const
        {
            return m_extensionFilter;
        }

        void FileDialogFileSelectView::setMultiSelectable(bool enable)
        {
            m_multiSelectable = enable;
            if (!m_multiSelectable)
            {
                m_selectedFiles.clear();
            }
        }

        bool FileDialogFileSelectView::isMultiSelectable() const
        {
            return m_multiSelectable;
        }

        void FileDialogFileSelectView::onDirectoryChange(DirectoryChangeCallback callback)
        {
            m_directoryChange = std::move(callback);
        }

        void FileDialogFileSelectView::onFileSelected(FileSelectedCallback callback)
        {
            m_fileSelected = std::move(callback);
        }

        void FileDialogFileSelectView::onFileActivated(FileActivatedCallback callback)
        {
            m_fileActivated = std::move(callback);
        }

        const std::filesystem::path &FileDialogFileSelectView::getSelectedFile() const
        {
            return m_selectedFile;
        }

        const std::vector<std::filesystem::path> &FileDialogFileSelectView::getSelectedFiles() const
        {
            return m_selectedFiles;
        }

        void FileDialogFileSelectView::draw()
        {
            if (m_currentDir.empty())
            {
                ImGui::TextDisabled("No directory selected.");
                return;
            }

            std::error_code ec;
            if (!std::filesystem::exists(m_currentDir, ec))
            {
                ImGui::TextDisabled("Directory not found.");
                return;
            }

            std::vector<std::filesystem::directory_entry> entries;
            for (std::filesystem::directory_iterator it(m_currentDir, ec); !ec && it != std::filesystem::directory_iterator(); ++it)
            {
                const auto &entry = *it;
                if (!m_extensionFilter.empty())
                {
                    std::error_code tmpEc;
                    if (entry.is_regular_file(tmpEc))
                    {
                        if (entry.path().extension() != m_extensionFilter)
                            continue;
                    }
                }
                entries.push_back(entry);
            }

            if (ec)
            {
                ImGui::TextDisabled("Unable to enumerate directory.");
                return;
            }

            std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {
                std::error_code aec, bec;
                const bool aIsDir = a.is_directory(aec);
                const bool bIsDir = b.is_directory(bec);
                if (aIsDir != bIsDir)
                {
                    return aIsDir && !bIsDir;
                }
                return PathToUTF8(a.path().filename()) < PathToUTF8(b.path().filename());
            });

            const ImGuiTableFlags tableFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY |
                                               ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter;

            ImVec2 tableSize = ImGui::GetContentRegionAvail();
            if (ImGui::BeginTable("##FileTable", 4, tableFlags, tableSize))
            {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch, 0.45f);
                ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableSetupColumn("Modified", ImGuiTableColumnFlags_WidthFixed, 150.0f);
                ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 90.0f);
                ImGui::TableHeadersRow();

                for (auto const &entry : entries)
                {
                    std::error_code entryEc;
                    const bool isDir = entry.is_directory(entryEc);
                    const bool isFile = entry.is_regular_file(entryEc);
                    ImGui::TableNextRow();

                    const std::string displayName = PathToUTF8(entry.path().filename());
                    const std::string entryId = PathToUTF8(entry.path());
                    std::string label = isDir ? ICON_FOLDER : ICON_FILE;
                    label.push_back(' ');
                    label += displayName;
                    label += "##";
                    label += entryId;

                    ImGui::TableSetColumnIndex(0);
                    const bool isSelected = m_multiSelectable
                                                ? std::find(m_selectedFiles.begin(), m_selectedFiles.end(), entry.path()) != m_selectedFiles.end()
                                                : (!isDir && entry.path() == m_selectedFile);
                    ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_SpanAllColumns |
                                                          ImGuiSelectableFlags_AllowDoubleClick;
                    const bool clicked = ImGui::Selectable(label.c_str(), isSelected, selectableFlags);
                    const bool doubleClicked = ImGui::IsItemHovered() &&
                                               ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);

                    if (clicked)
                    {
                        if (isDir)
                        {
                            m_selectedFile.clear();
                            m_selectedFiles.clear();
                        }
                        else
                        {
                            if (m_multiSelectable)
                            {
                                const bool additive = ImGui::GetIO().KeyCtrl || ImGui::GetIO().KeyShift;
                                if (!additive)
                                {
                                    m_selectedFiles.clear();
                                }
                                auto it = std::find(m_selectedFiles.begin(), m_selectedFiles.end(), entry.path());
                                if (additive && it != m_selectedFiles.end())
                                {
                                    m_selectedFiles.erase(it);
                                }
                                else
                                {
                                    m_selectedFiles.push_back(entry.path());
                                }
                                m_selectedFile = entry.path();
                            }
                            else
                            {
                                m_selectedFile = entry.path();
                            }

                            if (m_fileSelected)
                            {
                                m_fileSelected(entry.path());
                            }
                        }
                    }

                    if (isDir && doubleClicked)
                    {
                        if (m_directoryChange)
                        {
                            m_directoryChange(entry.path());
                        }
                    }
                    else if (!isDir && doubleClicked)
                    {
                        if (m_fileActivated)
                        {
                            m_fileActivated(entry.path());
                        }
                    }

                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(isDir ? "Folder" : "File");

                    ImGui::TableSetColumnIndex(2);
                    if (isDir || !isFile)
                    {
                        ImGui::TextUnformatted("-");
                    }
                    else
                    {
                        std::error_code timeEc;
                        auto lastWrite = entry.last_write_time(timeEc);
                        if (timeEc)
                        {
                            ImGui::TextUnformatted("-");
                        }
                        else
                        {
                            ImGui::TextUnformatted(FormatTime(lastWrite).c_str());
                        }
                    }

                    ImGui::TableSetColumnIndex(3);
                    if (isDir)
                    {
                        ImGui::TextUnformatted("-");
                    }
                    else
                    {
                        std::error_code sizeEc;
                        auto fileSize = entry.file_size(sizeEc);
                        if (sizeEc)
                        {
                            ImGui::TextUnformatted("-");
                        }
                        else
                        {
                            ImGui::TextUnformatted(FormatFileSize(fileSize).c_str());
                        }
                    }
                }

                ImGui::EndTable();
            }
        }

        // ----- FileDialogTopbar ----------------------------------------------------------------
        void FileDialogTopbar::setCurrentPath(const std::filesystem::path &path)
        {
            m_currentPath = path;
            syncBuffer();
        }

        void FileDialogTopbar::setNavigationCallbacks(NavigationCallback onBack,
                                                      NavigationCallback onForward,
                                                      NavigationCallback onUp,
                                                      NavigationCallback onRefresh)
        {
            m_onBack = std::move(onBack);
            m_onForward = std::move(onForward);
            m_onUp = std::move(onUp);
            m_onRefresh = std::move(onRefresh);
        }

        void FileDialogTopbar::onPathCommit(PathCommitCallback callback)
        {
            m_onPathCommit = std::move(callback);
        }

        void FileDialogTopbar::draw()
        {
            const float buttonSize = ImGui::GetFrameHeight();

            auto drawButton = [&](const char *label, NavigationCallback &cb, const char *tooltip) {
                if (ImGui::Button(label, ImVec2(buttonSize, buttonSize)))
                {
                    if (cb)
                    {
                        cb();
                    }
                }
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary) && tooltip)
                {
                    ImGui::SetTooltip("%s", tooltip);
                }
                ImGui::SameLine();
            };

            drawButton(ICON_BACK, m_onBack, "Back");
            drawButton(ICON_FORWARD, m_onForward, "Forward");
            drawButton(ICON_UP, m_onUp, "Up");
            drawButton(ICON_REFRESH, m_onRefresh, "Refresh");
            ImGui::SameLine();

            ImGui::SetNextItemWidth(-1);
            if (ImGui::InputText("##PathField", m_pathBuffer.data(), m_pathBuffer.size(),
                                 ImGuiInputTextFlags_EnterReturnsTrue))
            {
                if (m_onPathCommit)
                {
                    m_onPathCommit(std::filesystem::path(m_pathBuffer.data()));
                }
            }
        }

        void FileDialogTopbar::syncBuffer()
        {
            const std::string pathStr = PathToUTF8(m_currentPath);
#if defined(_WIN32)
            strncpy_s(m_pathBuffer.data(), m_pathBuffer.size(), pathStr.c_str(), _TRUNCATE);
#else
            std::snprintf(m_pathBuffer.data(), m_pathBuffer.size(), "%s", pathStr.c_str());
#endif
        }

        // ----- FileDialogBottombar -------------------------------------------------------------
        void FileDialogBottombar::setDirectory(const std::filesystem::path &path)
        {
            m_currentDirectory = path;
        }

        void FileDialogBottombar::setFilename(const std::string &filename)
        {
            m_filenameInput = filename;
            syncBuffer();
        }

        void FileDialogBottombar::setSelection(const std::vector<std::filesystem::path> &selection)
        {
            m_selectedFiles = selection;
            if (m_selectedFiles.empty())
            {
                return;
            }

            if (m_selectedFiles.size() == 1)
            {
                setFilename(PathToUTF8(m_selectedFiles.front().filename()));
            }
            else
            {
                std::ostringstream oss;
                for (size_t i = 0; i < m_selectedFiles.size(); ++i)
                {
                    if (i > 0)
                    {
                        oss << ", ";
                    }
                    oss << "\"" << PathToUTF8(m_selectedFiles[i].filename()) << "\"";
                }
                m_filenameInput = oss.str();
                syncBuffer();
            }
        }

        bool FileDialogBottombar::consumeConfirmed(std::string &outFilename)
        {
            if (!m_confirmed)
            {
                return false;
            }
            m_confirmed = false;
            outFilename = m_filenameInput;
            return true;
        }

        bool FileDialogBottombar::consumeCancelled()
        {
            if (!m_cancelled)
            {
                return false;
            }
            m_cancelled = false;
            return true;
        }

        void FileDialogBottombar::draw()
        {
            ImGui::TextUnformatted("File name:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-160.0f);
            if (ImGui::InputText("##FileNameInput", m_filenameBuffer.data(), m_filenameBuffer.size()))
            {
                m_filenameInput = m_filenameBuffer.data();
            }

            ImGui::SameLine();
            if (ImGui::Button("Open", ImVec2(70.0f, 0.0f)))
            {
                m_confirmed = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(70.0f, 0.0f)))
            {
                m_cancelled = true;
            }

            if (!m_currentDirectory.empty())
            {
                const std::string currentUtf8 = PathToUTF8(m_currentDirectory);
                ImGui::TextDisabled("Current: %s", currentUtf8.c_str());
            }
        }

        void FileDialogBottombar::syncBuffer()
        {
#if defined(_WIN32)
            strncpy_s(m_filenameBuffer.data(), m_filenameBuffer.size(), m_filenameInput.c_str(), _TRUNCATE);
#else
            std::snprintf(m_filenameBuffer.data(), m_filenameBuffer.size(), "%s", m_filenameInput.c_str());
#endif
        }

        // ----- FileDialog ----------------------------------------------------------------------
        FileDialog::FileDialog()
        {
            std::error_code ec;
            m_rootDirectory = std::filesystem::current_path(ec);
            if (ec)
            {
                m_rootDirectory = std::filesystem::path(".");
            }

            m_hierarchyView.refreshSystemRoots();
            connectCallbacks();
            setRootDirectory(m_rootDirectory);
        }

        void FileDialog::setRootDirectory(const std::filesystem::path &path)
        {
            std::error_code ec;
            if (path.empty() || !std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec))
            {
                return;
            }

            m_rootDirectory = path;
            m_hierarchyView.setRoot(m_rootDirectory);
            m_history.clear();
            m_historyCursor = -1;
            setCurrentDirectory(m_rootDirectory);
        }

        void FileDialog::setExtensionFilter(const std::filesystem::path &ext)
        {
            m_fileSelectView.setExtensionFilter(ext);
        }

        void FileDialog::enableMultiSelect(bool enable)
        {
            m_multiSelectEnabled = enable;
            m_fileSelectView.setMultiSelectable(enable);
        }

        void FileDialog::openDialog()
        {
            m_isVisible = true;
        }

        void FileDialog::setVisible(bool visible)
        {
            m_isVisible = visible;
        }

        bool FileDialog::isVisible() const
        {
            return m_isVisible;
        }

        void FileDialog::draw()
        {
            if (!m_isVisible)
            {
                return;
            }

            bool keepVisible = m_isVisible;
            ImGui::SetNextWindowSize(ImVec2(900, 540), ImGuiCond_FirstUseEver);
            const ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoCollapse;
            if (ImGui::Begin("File Dialog", &keepVisible, windowFlags))
            {
                const float topBarHeight = ImGui::GetFrameHeightWithSpacing() * 2.0f;
                const float bottomHeight = ImGui::GetFrameHeightWithSpacing() * 2.5f;

                ImGui::BeginChild("##Topbar", ImVec2(0.0f, topBarHeight), false, ImGuiWindowFlags_NoScrollbar);
                m_topbar.draw();
                ImGui::EndChild();

                ImGui::Separator();

                ImGui::BeginChild("##Body", ImVec2(0.0f, -bottomHeight), false);
                ImGui::BeginChild("##Hierarchy", ImVec2(220.0f, 0.0f), true);
                m_hierarchyView.draw();
                ImGui::EndChild();
                ImGui::SameLine();
                ImGui::BeginChild("##Files", ImVec2(0.0f, 0.0f), true);
                m_fileSelectView.draw();
                ImGui::EndChild();
                ImGui::EndChild();

                ImGui::Separator();

                ImGui::BeginChild("##BottomBar", ImVec2(0.0f, 0.0f), false);
                m_bottombar.draw();
                ImGui::EndChild();

                std::string confirmedName;
                if (m_bottombar.consumeConfirmed(confirmedName))
                {
                    std::vector<std::filesystem::path> selections;
                    const auto &selectedFiles = m_fileSelectView.getSelectedFiles();
                    if (m_multiSelectEnabled && !selectedFiles.empty())
                    {
                        selections.assign(selectedFiles.begin(), selectedFiles.end());
                    }
                    else if (!confirmedName.empty())
                    {
                        selections.emplace_back(confirmedName);
                    }
                    else if (!selectedFiles.empty())
                    {
                        selections.assign(selectedFiles.begin(), selectedFiles.end());
                    }
                    else
                    {
                        std::filesystem::path candidate = m_selectedFile;
                        if (candidate.empty())
                        {
                            candidate = m_currentDirectory;
                        }
                        selections.emplace_back(candidate);
                    }
                    confirmSelection(std::move(selections));
                }

                if (m_bottombar.consumeCancelled())
                {
                    m_lastConfirmedPath.clear();
                    m_lastConfirmedList.clear();
                    m_isVisible = false;
                }
            }
            ImGui::End();
            if (!keepVisible)
            {
                m_isVisible = false;
            }
        }

        std::filesystem::path FileDialog::open() const
        {
            return m_lastConfirmedPath;
        }

        const std::vector<std::filesystem::path> &FileDialog::openMultiple() const
        {
            return m_lastConfirmedList;
        }

        void FileDialog::setCurrentDirectory(const std::filesystem::path &path, bool pushHistory)
        {
            std::error_code ec;
            if (path.empty() || !std::filesystem::exists(path, ec) || !std::filesystem::is_directory(path, ec))
            {
                return;
            }

            if (pushHistory)
            {
                if (m_historyCursor + 1 < static_cast<int>(m_history.size()))
                {
                    m_history.resize(m_historyCursor + 1);
                }
                m_history.push_back(path);
                m_historyCursor = static_cast<int>(m_history.size()) - 1;
            }

            m_currentDirectory = path;
            m_hierarchyView.setSelectedDirectory(path);
            m_fileSelectView.setCurrentDirectory(path);
            m_topbar.setCurrentPath(path);
            m_bottombar.setDirectory(path);
        }

        void FileDialog::navigateBack()
        {
            if (m_historyCursor > 0)
            {
                --m_historyCursor;
                setCurrentDirectory(m_history[m_historyCursor], false);
            }
        }

        void FileDialog::navigateForward()
        {
            if (m_historyCursor + 1 < static_cast<int>(m_history.size()))
            {
                ++m_historyCursor;
                setCurrentDirectory(m_history[m_historyCursor], false);
            }
        }

        void FileDialog::navigateUp()
        {
            if (m_currentDirectory.has_parent_path())
            {
                setCurrentDirectory(m_currentDirectory.parent_path());
            }
        }

        void FileDialog::refresh()
        {
            setCurrentDirectory(m_currentDirectory, false);
        }

        void FileDialog::connectCallbacks()
        {
            m_hierarchyView.onDirectorySelected([this](const std::filesystem::path &path) {
                setCurrentDirectory(path);
            });

            m_fileSelectView.onDirectoryChange([this](const std::filesystem::path &path) {
                setCurrentDirectory(path);
            });

            m_fileSelectView.onFileSelected([this](const std::filesystem::path &file) {
                m_selectedFile = file;
                if (m_multiSelectEnabled)
                {
                    m_bottombar.setSelection(m_fileSelectView.getSelectedFiles());
                }
                else
                {
                    m_bottombar.setFilename(PathToUTF8(file.filename()));
                }
            });

            m_fileSelectView.onFileActivated([this](const std::filesystem::path &file) {
                if (m_multiSelectEnabled)
                {
                    const auto &selectedFiles = m_fileSelectView.getSelectedFiles();
                    if (!selectedFiles.empty())
                    {
                        std::vector<std::filesystem::path> bundle(selectedFiles.begin(), selectedFiles.end());
                        confirmSelection(std::move(bundle));
                        return;
                    }
                }
                confirmSelection(file);
            });

            m_topbar.setNavigationCallbacks(
                [this]() { navigateBack(); },
                [this]() { navigateForward(); },
                [this]() { navigateUp(); },
                [this]() { refresh(); });

            m_topbar.onPathCommit([this](const std::filesystem::path &path) {
                setCurrentDirectory(path);
            });
        }

        void FileDialog::confirmSelection(const std::filesystem::path &path)
        {
            std::vector<std::filesystem::path> single;
            if (!path.empty())
            {
                single.push_back(path);
            }
            confirmSelection(std::move(single));
        }

        void FileDialog::confirmSelection(std::vector<std::filesystem::path> paths)
        {
            if (paths.empty())
            {
                return;
            }

            m_lastConfirmedList.clear();
            for (auto &path : paths)
            {
                if (path.empty())
                {
                    continue;
                }

                std::filesystem::path candidate = path;
                if (!candidate.is_absolute())
                {
                    candidate = m_currentDirectory / candidate;
                }

                candidate = candidate.lexically_normal();
                m_lastConfirmedList.push_back(candidate);
            }

            if (m_lastConfirmedList.empty())
            {
                return;
            }

            m_lastConfirmedPath = m_lastConfirmedList.front();
            m_selectedFile = m_lastConfirmedList.back();
            m_bottombar.setFilename(PathToUTF8(m_selectedFile.filename()));
            m_isVisible = false;
        }
    }
}
