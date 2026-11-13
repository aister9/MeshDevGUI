# MeshDevGUI

CPU/GPU geometry deviation measurement and visualization. Uses cuBQL BVH builders/queries to compute per-vertex distances between a source mesh and a target mesh, then writes out colored PLY/OBJ for inspection. The GUI layer is integrated from ImGUIAX via CMake `FetchContent` rather than vendored into this repository. Implements the OBJ-based geometry deviation workflow from MeshDev (geometryDerivation): https://meshdev.sourceforge.net/

## Features
- CPU and GPU deviation calculator (cuBQL BVH).
- Color mapping with selectable palettes (jet, hot, cool, turbo, viridis, gray).
- Outputs colored PLY (binary) and OBJ.
- Command-line interface for source/target/output paths.
- ImGUIAX-based GUI fetched at configure time.
- OWL/OptiX mesh preview with orbit, pan, zoom, optional lighting, and deviation vertex colors.

## Build
Prerequisites:
- CMake 3.24+ (CUDA language enabled), C++17/CUDA17 toolchain
- NVIDIA CUDA Toolkit 11.8+
- NVIDIA OptiX 7.4+ (required by optixBaseLib)
- Git (to fetch cuBQL, ImGUIAX, and pinned NVIDIA OWL via FetchContent)

Configure/build example:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

> Note: the first configure downloads third-party sources through `FetchContent` (`cuBQL`, `ImGUIAX`, and ImGUIAX's GUI dependencies). Ensure `CMAKE_CUDA_ARCHITECTURES` matches your GPU. DLLs such as `cuBQL_cuda_float3` are copied next to the executable via a post-build step in `demo/CMakeLists.txt`.

## Run
Both executables are written to `output/`.

CLI (`MeshDevConsole`):
```bash
./output/MeshDevConsole <source.obj> <target.obj> <output.ply>
```
If arguments are omitted, it uses `dataset/scan25_cpuCleaned.obj`, `dataset/testGPUBinCleaned.obj`, and writes `dataset/deviation_output.{ply,obj}`. The CLI computes both CPU and GPU deviation, prints timing for each, and exports both PLY and OBJ outputs.

GUI (`MeshDevGUI`):
```bash
./output/MeshDevGUI
```
The GUI uses a docked workspace with **Mesh List**, **Scene View**, **Deviation**, and **Material** panels:

1. In **Mesh List**, use **Open OBJ files...** (multi-file selection) or enter an OBJ path and click **Add path**. Every triangle mesh in the file becomes a separate list entry.
2. Select a row to render it in **Scene View**. Select a reference row and click **Set Source**, then select another row and click **Set Target**. Display selection and Source/Target roles are independent.
3. In **Deviation**, choose CPU, GPU, or CPU + GPU and click **Calculate Deviation**. The calculation stores raw distances on the target and selects it. It does not encode or write a file. CPU + GPU retains both result arrays.
4. In **Material**, select the CPU/GPU result, color map, normalization method, and sigma scale. These are per-mesh settings; edits update the scene colors without recalculating distances or resetting the camera. The panel shows raw min/max/mean and the color legend. Turn off **Show deviation colors** to use a base color.
5. Set the output filename and click **Save OBJ** to export the selected mesh with its current material colors. The extension is normalized to `.obj`; loaded input files cannot be overwritten. The saved/not-saved indicator tracks subsequent changes. Workspace state is in memory and is discarded on exit.
6. In **Scene View**, click **Save PNG...**, choose an output path, then **Save** to capture the current camera, colors and lighting without UI panels. Images use the viewport render resolution (up to 2048 pixels on the longest side). Existing PNGs require **Replace existing PNG** to be checked. Saving an image does not mark the mesh as exported.

Scene controls: left drag rotates, right/middle drag pans, the wheel zooms, and **Fit** frames the selected mesh. The first mesh is fitted automatically; switching selection preserves the current camera. **Lighting** is off by default to display unmodified deviation colors. Renderer errors appear inside Scene View and do not disable calculation, material editing, or export. Removing a mesh clears its selection and Source/Target roles; already computed results on other meshes keep their source name and distance snapshot.

For Visual Studio builds, run from `output/Release` (the VS Code launch configuration does this). The file dialog starts in `../../dataset` when that directory exists. CLI PLY export is unchanged.

> Note: PLY input is currently experimental; primary supported interchange format is OBJ.

## Repository layout (key files)
- `demo/demo.cpp` – `MeshDevConsole` CLI entry; writes colored PLY/OBJ.
- `demo/MeshDevGUIDemo.cpp` – `MeshDevGUI` entry built on `visualizerLib` from ImGUIAX.
- `libs/geometry/GeometryDeviation.h` – deviation API; color maps.
- `include/geometry/GeometryDeviationHost.cpp` – CPU implementation.
- `include/geometry/GeometryDeviationDevice.cu` – GPU stub/impl (requires cuBQL CUDA).
- `libs/geometry/CMakeLists.txt` – geometryLib + CUDA source setup.
- `demo/CMakeLists.txt` – app targets and GUI linkage.
- `CMakeLists.txt` – top-level `FetchContent` wiring for external dependencies.

## License
This project is licensed under the Apache License 2.0 (see `LICENSE`).

Third-party:
- cuBQL (Apache 2.0, NVIDIA): fetched via FetchContent; follows its license.
- ImGUIAX (Apache 2.0): fetched via FetchContent; follows its license.
- NVIDIA OWL (Apache 2.0): fetched via FetchContent; include its license notices when distributing linked binaries.
- Dear ImGui + backends (MIT License): see `LICENSE-3rdparty/IMGUI_LICENSE.txt`.
- OptiX and CUDA: subject to NVIDIA EULA/terms, not included here; you must obtain and accept those separately.

If you distribute binaries that bundle cuBQL, include its license notice consistent with Apache 2.0 requirements.

## Dataset
- Large OBJ assets are **not tracked in git** (each >25 MB). Download from the shared folder and place under `dataset/`:
  - `dataset/scan25_cpuCleaned.obj`
  - `dataset/testGPUBinCleaned.obj`
- Download link (Google Drive, view-only/download-only): [here](https://drive.google.com/drive/folders/1zp1rCFbLHC5P_xOCY6sGU30L499t8gZD?usp=sharing)
- Provenance: derived from DTU Dataset scan25 reconstructed via OpenMVS; ensure DTU license compliance for your use.

## Windows recovery and launching the GUI

`demo/MeshDevGUIDemo.cpp` is the source file; the CMake target is **MeshDevGUI**, and the executable is **MeshDevGUI.exe**. For a Visual Studio Release build, run from the executable directory so the GUI's `../../dataset/` defaults resolve correctly:

```powershell
Set-Location 'E:\visual studio project\MeshDevGUI\output\Release'
.\MeshDevGUI.exe
```

The current Windows configuration uses **Visual Studio 2026 Community** (MSVC 19.52), **CUDA Toolkit 13.5.37**, and **CMake 4.5.0**, with Windows SDK 10.0.26100.0. VS must include Desktop development with C++, a Windows SDK, and CUDA Visual Studio integration. The preset selects `Visual Studio 18 2026`, `x64`, CUDA `13.5`, and architecture `75` (RTX 2070 SUPER).

From the project root:

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' --preset vs2026-cuda135
& 'C:/Program Files/CMake/bin/cmake.exe' --build --preset vs2026-release
& 'C:/Program Files/CMake/bin/ctest.exe' --preset vs2026-release
Push-Location ./output/Release
try { ./MeshDevGUI.exe } finally { Pop-Location }
```

`CMakePresets.json` provides the CUDA environment even if the current terminal inherited an old PATH. It builds in `build-vs2026-cuda135`; do not reuse the old `build/` compiler cache. Change the preset's toolkit path or architecture if your installation differs. CMake 4.2+ is required for the VS 2026 generator. The project itself still supports CMake 3.24+ with older supported generators.

On this restored machine, ignored `CMakeUserPresets.json` also provides `vs2026-cuda135-local` and `vs2026-release-local`, which reuse the dependency source trees in `build/_deps` without downloading newer revisions. Use those names for the local configure/build/test commands while those sources remain available.

The build copies the matching CUDA runtime DLL and cuBQL DLL beside the GUI executable. CUDA 13.5 stores its runtime under `bin/x64`, while CUDA 12 used `bin`. `0xc0000135` generally means a runtime dependency cannot be loaded; also ensure the Microsoft VC++ x64 runtime is installed.

VS Code workspace settings use the installed CMake executable and presets, remove the obsolete vcpkg path, and set the debugger working directory to `output/Release`. After installing tools, run **Developer: Reload Window**, select **CMake: Select Configure Preset** ? `vs2026-cuda135-local`, then select the matching Release build preset. New integrated terminals also receive the CUDA/CMake paths from the workspace settings. In Visual Studio, the generated GUI project uses the executable directory as its debugger working directory.

Compatibility fixes for this toolchain:

- Enable MSVC `/Zc:preprocessor`, including NVCC host compilation, for CUDA 13's CCCL headers used by cuBQL.
- Enable C at the root project so GLEW gets the same MSVC DLL runtime as the C++ targets, avoiding `LNK4098`.
- Use UTF-8 source decoding for project and dependency headers on Korean Windows.
- Use the standard scalar `fminf`/`fmaxf` functions instead of redefining MSVC intrinsics in `helper_math.h`.
- Populate the local OWL/ImGUIAX integration through `FetchContent_MakeAvailable`. ImGUIAX's own GLEW/ImGui population still emits CMake deprecation warnings.

Local validation on 2026-11-14: VS 2026 / CUDA 13.5 / CMake 4.5 Release build succeeds. With NVIDIA driver **621.24** on the RTX 2070 SUPER, all **six CTest tests pass**, including OWL rendering/material updates, CPU/GPU workspace workflows, and PNG round-trip checks (dimensions, orientation, RGBA values, Unicode paths and write failures). The GUI window starts, but screenshot-based visual validation was blocked by the capture helper's `0x887A0005` device-removed error.

The previous driver 596.36 rejected CUDA 13.5's PTX 9.5; upgrading the display driver resolved that renderer failure. CUDA 13.5 PTX requires a compatible **R620 or later** driver. See [CUDA driver requirements](https://docs.nvidia.com/cuda/cuda-toolkit-release-notes/) and [PTX compatibility limitations](https://docs.nvidia.com/deploy/cuda-compatibility/minor-version-compatibility.html).

This project exercises cuBQL and OWL/OptiX. It does not build gDel3D, so these checks do not establish gDel3D compatibility with CUDA 13.5.

## GUI Integration
The GUI executable links against `visualizerLib` provided by `ImGUIAX`. The dependency is fetched during CMake configure, so this repository does not need a checked-in copy of `libs/Visualizer`.

The application follows the existing `libs/` headers and `include/` implementation convention:

```text
main -> MeshDevApplication (assembly and dock layout)
          MeshWorkspace (meshes, stable IDs, roles, results, materials)
          Stage
            WorkspaceLayout
            MeshListPanel   -> load/select/remove, assign Source/Target
            DeviationPanel  -> compute raw distances
            MaterialPanel   -> color mapping controls and explicit Save
            SceneView       -> selected mesh, camera, OpenGL texture
              MeshRenderer  -> OwlRenderer (OWL/OptiX)
```

- `MeshWorkspace.cpp` owns domain state and coordinates calculations; `MeshWorkspaceIO.cpp` handles imports and explicit exports. `DeviationMaterial.cpp` maps raw distances to colors without changing the result data.
- `meshWorkspaceLib` has no ImGui, OpenGL, or OWL dependency, allowing workflow tests without a renderer.
- `meshViewerLib` implements each panel separately. `SceneView` watches the selected mesh ID and revision; changing a material uploads only the color buffer and preserves the BVH/camera.
- `meshAppLib` assembles the components and default dock layout. `demo/MeshDevGUIDemo.cpp` only calls the application launcher.
- `meshRendererLib` remains independent of ImGui/OpenGL. Its camera uses `float3` and the existing `helper_math.h` operations. Device programs are compiled to embedded PTX.
- OWL is pinned to commit `a701aecccfb6a6081f3b114bcabb1640b6d39ab0` and linked statically, using the shared OptiX 7.4 headers. Deviation computation still uses cuBQL.
- Rendering occurs on mesh/material/camera/viewport changes, using a synchronized host-pinned framebuffer upload to OpenGL (2048-pixel longest-side limit). Stage destroys its panels before the GL context. No fetched ImGUIAX source is modified.

To build just the console without downloading GUI/OWL dependencies, configure with `-DMESHDEV_BUILD_GUI=OFF`.

Optional renderer validation (requires a supported NVIDIA GPU and OptiX driver):

```bash
cmake -S . -B build -DMESHDEV_BUILD_RENDERER_TESTS=ON
cmake --build build --config Release --target MeshDevGUI MeshDevCameraTest MeshDevRendererSmoke MeshDevGeometrySmoke MeshDevWorkspaceTest MeshDevFrameImageTest
ctest --test-dir build -C Release --output-on-failure
```

Workspace tests cover multi-mesh import, roles/selection, raw-result preservation, per-mesh material changes, CPU/GPU result selection, no writes during calculation/material edits, exact exported colors, failed saves, input protection, and removal. The CPU/GPU geometry test checks analytic face/edge/vertex distances and repeated cuBQL queries. The camera test covers landscape/portrait fitting and camera interactions. The headless GPU test covers hit/miss colors, material-only updates, framebuffer resize, mesh replacement, invalid inputs, and repeated resource lifetimes. Camera tests can also be built independently using `cmake -S tests -B build-camera`; they require CUDA Toolkit headers for `helper_math.h`, but do not require a GPU, CUDA device compilation, or OWL.

## To Do
- Simultaneous multi-mesh scenes and source/target overlays.
- Persisting workspace sessions and raw deviation data.
- Background deviation jobs and progress/cancellation.
- Direct CUDA/OpenGL interop for larger interactive previews.
