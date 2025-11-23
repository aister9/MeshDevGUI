#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <utility>
#include <filesystem>
#include <system_error>
#include <codecvt>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifndef IMGUI_IMPL_OPENGL_LOADER_GLEW
#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#endif
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "UI/Components.h"

namespace SPIN
{
    namespace Visualizer
    {
        class Stage
        {
        public:
            using SceneRenderer = std::function<void(Stage &, float)>;

            Stage() = default;

            Stage(const char *title, int width, int height, const char *glslVersion = "#version 460 core")
            {
                create(title, width, height, glslVersion);
            }

            ~Stage()
            {
                shutdown();
            }

            Stage(const Stage &) = delete;
            Stage &operator=(const Stage &) = delete;

            void create(const char *title, int width, int height, const char *glslVersion = "#version 460 core")
            {
                if (m_window)
                {
                    throw std::runtime_error("Stage window already created.");
                }

                glfwSetErrorCallback(glfwErrorHandler);

                if (!m_glfwInitialized)
                {
                    if (!glfwInit())
                    {
                        throw std::runtime_error("Failed to initialize GLFW.");
                    }
                    m_glfwInitialized = true;
                }

                glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
                glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
                glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
                glfwWindowHint(GLFW_DEPTH_BITS, 24);
#if defined(__APPLE__)
                glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

                m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
                if (!m_window)
                {
                    throw std::runtime_error("Failed to create GLFW window.");
                }

                glfwMakeContextCurrent(m_window);
                glfwSwapInterval(1); // vsync

                glewExperimental = GL_TRUE;
                GLenum glewErr = glewInit();
                if (glewErr != GLEW_OK)
                {
                    std::string message = "GLEW initialization failed: ";
                    message += reinterpret_cast<const char *>(glewGetErrorString(glewErr));
                    throw std::runtime_error(message);
                }

                // ImGui context
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                m_imguiContext = ImGui::GetCurrentContext();
                ImGuiIO &io = ImGui::GetIO();
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
                // After support
                io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
                ImGui::StyleColorsDark();
                setupFonts();

                ImGui_ImplGlfw_InitForOpenGL(m_window, true);
                ImGui_ImplOpenGL3_Init(glslVersion);
                m_imguiInitialized = true;
                m_lastFrameTime = glfwGetTime();

                glEnable(GL_DEPTH_TEST);
            }

            template <typename NodeType, typename... Args>
            NodeType &emplaceDrawable(Args &&...args)
            {
                auto node = std::make_unique<NodeType>(std::forward<Args>(args)...);
                NodeType &ref = *node;
                m_drawables.emplace_back(std::move(node));
                return ref;
            }

            void setSceneRenderer(SceneRenderer renderer)
            {
                m_sceneRenderer = std::move(renderer);
            }

            void setClearColor(const ImVec4 &color)
            {
                m_clearColor = color;
            }

            GLFWwindow *getWindow() const { return m_window; }

            bool shouldClose() const
            {
                return m_window && glfwWindowShouldClose(m_window);
            }

            void show()
            {
                if (!m_window)
                {
                    throw std::runtime_error("Stage::show called before create.");
                }

                while (!shouldClose())
                {
                    pollEvents();
                    renderFrame();
                }
            }

        private:
            static void glfwErrorHandler(int error, const char *description)
            {
                std::cerr << "GLFW Error (" << error << "): " << description << std::endl;
            }

            void pollEvents() const
            {
                glfwPollEvents();
            }

            void renderFrame()
            {
                const double now = glfwGetTime();
                const float deltaTime = static_cast<float>(now - m_lastFrameTime);
                m_lastFrameTime = now;

                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                for (auto &drawable : m_drawables)
                {
                    drawable->draw();
                }

                ImGui::Render();

                int fbWidth = 0;
                int fbHeight = 0;
                glfwGetFramebufferSize(m_window, &fbWidth, &fbHeight);
                if (fbWidth == 0 || fbHeight == 0)
                {
                    return;
                }

                glViewport(0, 0, fbWidth, fbHeight);
                glClearColor(m_clearColor.x, m_clearColor.y, m_clearColor.z, m_clearColor.w);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                if (m_sceneRenderer)
                {
                    m_sceneRenderer(*this, deltaTime);
                }

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                glfwSwapBuffers(m_window);
            }

            void shutdown()
            {
                m_drawables.clear();

                if (m_imguiInitialized)
                {
                    ImGui_ImplOpenGL3_Shutdown();
                    ImGui_ImplGlfw_Shutdown();
                    ImGui::DestroyContext(m_imguiContext);
                    m_imguiContext = nullptr;
                    m_imguiInitialized = false;
                }

                if (m_window)
                {
                    glfwDestroyWindow(m_window);
                    m_window = nullptr;
                }

                if (m_glfwInitialized)
                {
                    glfwTerminate();
                    m_glfwInitialized = false;
                }
            }

            GLFWwindow *m_window = nullptr;
            ImGuiContext *m_imguiContext = nullptr;
            bool m_glfwInitialized = false;
            bool m_imguiInitialized = false;
            ImVec4 m_clearColor = ImVec4(0.1f, 0.1f, 0.1f, 1.0f);
            double m_lastFrameTime = 0.0;
            SceneRenderer m_sceneRenderer;
            std::vector<std::unique_ptr<Components>> m_drawables;

            void setupFonts();
            bool tryLoadFontCandidates(const std::vector<std::filesystem::path> &candidates,
                                       float size,
                                       bool merge,
                                       const ImWchar *ranges);
            static std::string pathToUTF8(const std::filesystem::path &path);
        };
    }
}

namespace SPIN
{
    namespace Visualizer
    {
        inline std::string Stage::pathToUTF8(const std::filesystem::path &path)
        {
#if defined(_WIN32)
            static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            return converter.to_bytes(path.wstring());
#else
            return path.string();
#endif
        }

        inline bool Stage::tryLoadFontCandidates(const std::vector<std::filesystem::path> &candidates,
                                                 float size,
                                                 bool merge,
                                                 const ImWchar *ranges)
        {
            if (candidates.empty())
            {
                return false;
            }

            ImGuiIO &io = ImGui::GetIO();
            ImFontConfig config;
            config.MergeMode = merge;
            config.PixelSnapH = true;
            config.OversampleH = 2;
            config.OversampleV = 2;

            for (const auto &candidate : candidates)
            {
                std::error_code ec;
                if (candidate.empty() || !std::filesystem::exists(candidate, ec))
                {
                    continue;
                }
                const std::string fontPath = pathToUTF8(candidate);
                if (fontPath.empty())
                {
                    continue;
                }
                if (io.Fonts->AddFontFromFileTTF(fontPath.c_str(), size, &config, ranges))
                {
                    return true;
                }
            }
            return false;
        }

        inline void Stage::setupFonts()
        {
            ImGuiIO &io = ImGui::GetIO();
            io.Fonts->Clear();
            constexpr float baseFontSize = 18.0f;
            io.Fonts->AddFontDefault();

            const std::vector<std::filesystem::path> cjkCandidates = {
#if defined(_WIN32)
                "C:/Windows/Fonts/malgun.ttf",
                "C:/Windows/Fonts/unifont.ttf",
                "C:/Windows/Fonts/gulim.ttc"
#elif defined(__APPLE__)
                "/System/Library/Fonts/AppleSDGothicNeo.ttc",
                "/System/Library/Fonts/Supplemental/AppleGothic.ttf"
#else
                "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc",
                "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.otf",
                "/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc"
#endif
            };
            const bool cjkLoaded = tryLoadFontCandidates(cjkCandidates, baseFontSize, true, io.Fonts->GetGlyphRangesKorean());

            const std::vector<std::filesystem::path> emojiCandidates = {
#if defined(_WIN32)
                "C:/Windows/Fonts/seguiemj.ttf",
                "C:/Windows/Fonts/seguisym.ttf",
                "C:/Windows/Fonts/Symbola.ttf"
#elif defined(__APPLE__)
                "/System/Library/Fonts/Apple Color Emoji.ttc",
                "/System/Library/Fonts/Supplemental/AppleGothic.ttf",
                "/System/Library/Fonts/Supplemental/Symbols.ttf"
#else
                "/usr/share/fonts/truetype/noto/NotoEmoji-Regular.ttf",
                "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf",
                "/usr/share/fonts/truetype/emojione/emojione-android.ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"
#endif
            };
            static const ImWchar emojiRanges[] = {
                0x0020, 0x00FF,
                0x2000, 0x206F,
                0x2100, 0x214F,
                0x2190, 0x21FF,
                0x2300, 0x23FF,
                0x2600, 0x27FF,
                0x2900, 0x297F,
                0x1F000, 0x1FFFF,
                0
            };
            const bool emojiLoaded = tryLoadFontCandidates(emojiCandidates, baseFontSize, true, emojiRanges);

            if (!cjkLoaded)
            {
                std::cout << "[Stage] Warning: CJK font not found; UI may lack Hangul glyphs.\n";
            }
            if (!emojiLoaded)
            {
                std::cout << "[Stage] Warning: Emoji font not found; falling back to ASCII icons.\n";
            }

            io.Fonts->Build();
        }
    }
}
