# Experimental Visualizer (Codex Prototype)

This folder contains the in-progress GUI/renderer work for the MeshDev visualizer. It bundles the Stage (GLFW/GLEW + Dear ImGui bootstrap), compositor components, and FileDialog.

> **Note:** The code in this folder was co-authored with the Codex CLI assistant.

## Structure
- Stage.h: owns the GLFW window, ImGui lifecycle, and per-frame render loop.
- Application.h: base class + `launch<TApp>(args)` helper (JavaFX-style). Derive from `Application`, override `start(Stage&)`, and call `launch<MyApp>(argc, argv)` from `main`.
- UI/Components.h: base interface for ImGui components.
- UI/FileDialog.*: Dear ImGui file picker supporting multi-select + filters.

## Prerequisites
- GLFW, GLEW, Dear ImGui (linked through vcpkg or your toolchain).
- OpenGL 4.6 context.

## Building / Running
- CMake target MeshDevGUI (see demo/CMakeLists.txt) compiles the experimental GUI.
- Executable is emitted to output/.

## License
- All visualizer-specific code follows the main project license (Apache 2.0).
- Dear ImGui and its backends ship under the MIT License; see LICENSE-3rdparty/IMGUI_LICENSE.txt.
