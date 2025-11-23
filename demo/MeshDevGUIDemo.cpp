#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <type_traits>
#include <fstream>
#include <algorithm>

#include "Visualizer/Application.h"
#include "Visualizer/UI/FileDialog.h"

#include "3rdParty/TimeChecker.h"

#include "Object_t.h"
#include "GeometryDeviation.h"

using SPIN::Visualizer::Components;
using SPIN::Visualizer::Stage;

namespace
{
    std::string PathToDisplayString(const std::filesystem::path &path)
    {
#if defined(_WIN32)
        return path.u8string();
#else
        return path.string();
#endif
    }

    bool triangleMeshToOBJ(const TriangleMesh &mesh, const std::vector<float3> &vColorMaps, const std::string &filename)
    {
        std::ofstream objOut(filename);
        if (!objOut)
        {
            std::cerr << "triangleMeshToOBJ: cannot open file: " << filename << "\n";
            return false;
        }

        if (vColorMaps.size() != mesh.vertex.size())
        {
            std::cerr << "triangleMeshToOBJ: color map size mismatch\n";
            return false;
        }

        // Write vertices with colors
        for (size_t i = 0; i < mesh.vertex.size(); ++i)
        {
            const float3 &v = mesh.vertex[i];
            const float3 &c = vColorMaps[i];
            objOut << "v " << v.x << " " << v.y << " " << v.z << " "
                   << c.x << " " << c.y << " " << c.z << "\n";
        }

        // Write faces
        for (const uint3 &f : mesh.index)
        {
            // OBJ format uses 1-based indexing
            objOut << "f " << (f.x + 1) << " " << (f.y + 1) << " " << (f.z + 1) << "\n";
        }

        return true;
    }

    float computeMedianEdgeLength(const TriangleMesh &mesh)
    {
        std::vector<float> edges;
        edges.reserve(mesh.index.size() * 3);

        for (const uint3 &f : mesh.index)
        {
            const float3 &v0 = mesh.vertex[f.x];
            const float3 &v1 = mesh.vertex[f.y];
            const float3 &v2 = mesh.vertex[f.z];

            float e0 = length(v1 - v0);
            float e1 = length(v2 - v1);
            float e2 = length(v0 - v2);

            edges.push_back(e0);
            edges.push_back(e1);
            edges.push_back(e2);
        }

        if (edges.empty())
            return 1.0f;

        std::sort(edges.begin(), edges.end());
        return edges[edges.size() / 2]; // median
    }
}

class MeshDevGUIPanel final : public Components
{
public:
    MeshDevGUIPanel()
    {
        resetPaths();
        m_statusMessage = "Idle";
    }

    void draw() override
    {
        ImGui::SetNextWindowPos(ImVec2(32, 32), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(520, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("MeshDev GUI Panel"))
        {
            ImGui::TextUnformatted("Dataset Paths");
            ImGui::Separator();

            drawPathRow("Source OBJ", m_sourcePath, DialogPurpose::Source, ".obj");
            drawPathRow("Target OBJ", m_targetPath, DialogPurpose::Target, ".obj");
            drawPathRow("Output Path", m_outputPath, DialogPurpose::Output, "");

            if (ImGui::Button("Batch Select Meshes..."))
            {
                openDialog(DialogPurpose::Batch, ".obj", true);
            }

            if (!m_recentSelection.empty())
            {
                ImGui::Spacing();
                ImGui::Text("Recent batch selection:");
                ImGui::BeginChild("##RecentBatch", ImVec2(0, 100), true);
                for (const auto &path : m_recentSelection)
                {
                    ImGui::BulletText("%s", PathToDisplayString(path).c_str());
                }
                ImGui::EndChild();
            }

            ImGui::Spacing();
            ImGui::TextUnformatted("Options");
            ImGui::Separator();
            static const char *kExecutionModes[] = {"CPU", "GPU", "CPU + GPU"};
            ImGui::Combo("Execution Mode", &m_computeMode, kExecutionModes, IM_ARRAYSIZE(kExecutionModes));
            ImGui::SliderFloat("Sigma Scale", &m_sigmaScale, 0.1f, 5.0f, "%.2f");

            static const char *kColorMaps[] = {"Turbo", "Viridis", "Hot", "Cool", "Gray"};
            ImGui::Combo("Color Map", &m_colorMapIndex, kColorMaps, IM_ARRAYSIZE(kColorMaps));

            ImGui::Spacing();
            if (ImGui::Button("Run Deviation"))
            {
                logRunRequest();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset"))
            {
                resetPaths();
                m_recentSelection.clear();
                m_statusMessage = "Reset to defaults.";
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextWrapped("Status: %s", m_statusMessage.c_str());
        }
        ImGui::End();

        m_fileDialog.draw();
        processDialogResult();
    }

private:
    enum class DialogPurpose
    {
        None,
        Source,
        Target,
        Output,
        Batch
    };

    void drawPathRow(const char *label,
                     std::array<char, 520> &buffer,
                     DialogPurpose purpose,
                     const char *filterExtension = ".obj")
    {
        ImGui::PushID(label);
        ImGui::InputText(label, buffer.data(), buffer.size());
        ImGui::SameLine();
        std::string buttonLabel = std::string("...##") + label;
        if (ImGui::Button(buttonLabel.c_str()))
        {
            openDialog(purpose, filterExtension, false);
        }
        ImGui::PopID();
    }

    void openDialog(DialogPurpose purpose, const char *filter, bool allowMulti)
    {
        m_pendingPurpose = purpose;
        if (filter && *filter)
        {
            m_fileDialog.setExtensionFilter(std::filesystem::path(filter));
        }
        else
        {
            m_fileDialog.setExtensionFilter(std::filesystem::path());
        }
        m_fileDialog.enableMultiSelect(allowMulti);
        m_fileDialog.openDialog();
    }

    void processDialogResult()
    {
        const auto &selected = m_fileDialog.openMultiple();
        if (selected == m_lastDialogResult)
        {
            return;
        }

        m_lastDialogResult = selected;
        if (selected.empty())
        {
            return;
        }

        const std::filesystem::path &first = selected.front();
        switch (m_pendingPurpose)
        {
        case DialogPurpose::Source:
            setPathBuffer(m_sourcePath, first);
            break;
        case DialogPurpose::Target:
            setPathBuffer(m_targetPath, first);
            break;
        case DialogPurpose::Output:
            setPathBuffer(m_outputPath, first);
            break;
        case DialogPurpose::Batch:
            m_recentSelection = selected;
            break;
        default:
            break;
        }
        m_pendingPurpose = DialogPurpose::None;
    }
    void logRunRequest()
    {
        const std::filesystem::path source(m_sourcePath.data());
        const std::filesystem::path target(m_targetPath.data());

        Object_t objA(source.string());
        Object_t objB(target.string());
        if (objA.model->meshes.empty() || objB.model->meshes.empty())
        {
            m_statusMessage = "Error: Source or Target mesh is empty.";
            std::cout << "[MeshDevGUIPanel] " << m_statusMessage << std::endl;
            return;
        }

        TriangleMesh &sourceMesh = *objA.model->meshes[0];
        TriangleMesh &targetMesh = *objB.model->meshes[0];
        float sigma = computeMedianEdgeLength(sourceMesh) * m_sigmaScale;

        std::filesystem::path outputBase(m_outputPath.data());
        if (outputBase.extension().empty())
        {
            outputBase.replace_extension(".ply");
        }

        auto colorMap = buildColorMap();
        auto buildOutputPath = [](const std::filesystem::path &base, const char *suffix) {
            std::filesystem::path stem = base;
            stem.replace_extension();
            std::filesystem::path ext = base.extension();
            if (ext.empty())
            {
                ext = ".ply";
            }
            std::filesystem::path composed = stem;
            composed += suffix;
            composed += ext.string();
            return composed;
        };

        auto runMode = [&](const char *label,
                           auto tagConstant,
                           const std::filesystem::path &outPath) -> bool {
            using TagType = typename decltype(tagConstant)::value_type;
            constexpr TagType tagValue = tagConstant.value;
            GeometryDeviation<tagValue> geomDev(sourceMesh, targetMesh);
            geomDev.computeDeviation();
            const auto &rawDeviations = geomDev.getDeviations();
            if (rawDeviations.size() != targetMesh.vertex.size())
            {
                m_statusMessage = std::string("Deviation size mismatch for ") + label;
                std::cout << "[MeshDevGUIPanel] " << m_statusMessage << std::endl;
                return false;
            }

            std::vector<float> normalized = rawDeviations;
            for (float &d : normalized)
            {
                d /= sigma;
            }

            std::vector<float3> colors(normalized.size());
            for (size_t i = 0; i < normalized.size(); ++i)
            {
                colors[i] = GeometryDeviationBase::deviation2Color(normalized[i], 256, colorMap);
            }

            if (!triangleMeshToOBJ(targetMesh, colors, outPath.string()))
            {
                m_statusMessage = std::string("Failed to write OBJ for ") + label;
                std::cout << "[MeshDevGUIPanel] " << m_statusMessage << std::endl;
                return false;
            }

            std::ostringstream oss;
            //oss << label << " deviation complete in " << elapsedMs << " ms -> " << outPath;
            m_statusMessage = oss.str();
            std::cout << "[MeshDevGUIPanel] " << m_statusMessage << std::endl;
            return true;
        };

        bool ranSomething = false;
        if (m_computeMode == 0 || m_computeMode == 2)
        {
            ranSomething |= runMode("CPU",
                                    std::integral_constant<SPIN::ExecTag, SPIN::ExecTag::HOST>{},
                                    buildOutputPath(outputBase, "_cpu"));
        }
        if (m_computeMode == 1 || m_computeMode == 2)
        {
            ranSomething |= runMode("GPU",
                                    std::integral_constant<SPIN::ExecTag, SPIN::ExecTag::DEVICE>{},
                                    buildOutputPath(outputBase, "_gpu"));
        }

        if (!ranSomething)
        {
            m_statusMessage = "No execution mode selected.";
            std::cout << "[MeshDevGUIPanel] " << m_statusMessage << std::endl;
        }
    }

    void setPathBuffer(std::array<char, 520> &buffer, const std::filesystem::path &path)
    {
        const std::string text = PathToDisplayString(path);
#if defined(_WIN32)
        strncpy_s(buffer.data(), buffer.size(), text.c_str(), _TRUNCATE);
#else
        std::snprintf(buffer.data(), buffer.size(), "%s", text.c_str());
#endif
    }

    void resetPaths()
    {
        setPathBuffer(m_sourcePath, std::filesystem::path("dataset/scan25_cpuCleaned.obj"));
        setPathBuffer(m_targetPath, std::filesystem::path("dataset/testGPUBinCleaned.obj"));
        setPathBuffer(m_outputPath, std::filesystem::path("dataset/deviation_output.ply"));
        m_statusMessage = "Idle";
    }

    std::vector<float3> buildColorMap() const
    {
        switch (m_colorMapIndex)
        {
        case 0:
            return SPIN::ColorMapLibrary::TurboColorMap();
        case 1:
            return SPIN::ColorMapLibrary::ViridisColorMap();
        case 2:
            return SPIN::ColorMapLibrary::HotColorMap();
        case 3:
            return SPIN::ColorMapLibrary::CoolColorMap();
        case 4:
        default:
            return SPIN::ColorMapLibrary::GrayColorMap();
        }
    }

    SPIN::Visualizer::FileDialog m_fileDialog;
    DialogPurpose m_pendingPurpose = DialogPurpose::None;
    std::array<char, 520> m_sourcePath{};
    std::array<char, 520> m_targetPath{};
    std::array<char, 520> m_outputPath{};
    int m_computeMode = 2; // 0: CPU, 1: GPU, 2: CPU+GPU
    float m_sigmaScale = 1.0f;
    int m_colorMapIndex = 0;
    std::vector<std::filesystem::path> m_recentSelection;
    std::vector<std::filesystem::path> m_lastDialogResult;
    std::string m_statusMessage;
};

class FileDialogDemoApplication final : public SPIN::Visualizer::Application
{
public:
    const char *title() const override { return "MeshDev GUI (Stage prototype)"; }

    void start(Stage &stage) override
    {
        stage.setClearColor(initialClearColor());
        stage.emplaceDrawable<MeshDevGUIPanel>();

        stage.setSceneRenderer([](Stage &, float)
                               {
                                   // Placeholder: wire the GLSL renderer here (SimpleRenderer, etc.).
                               });
    }
};

int main(int argc, char **argv)
{
    return SPIN::Visualizer::launch<FileDialogDemoApplication>(argc, argv);
}
