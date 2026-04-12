# MeshDevGUI

CPU/GPU geometry deviation measurement and visualization. Uses cuBQL BVH builders/queries to compute per-vertex distances between a source mesh and a target mesh, then writes out colored PLY/OBJ for inspection. The GUI layer is integrated from ImGUIAX via CMake `FetchContent` rather than vendored into this repository. Implements the OBJ-based geometry deviation workflow from MeshDev (geometryDerivation): https://meshdev.sourceforge.net/

## Features
- CPU and GPU deviation calculator (cuBQL BVH).
- Color mapping with selectable palettes (jet, hot, cool, turbo, viridis, gray).
- Outputs colored PLY (binary) and OBJ.
- Command-line interface for source/target/output paths.
- ImGUIAX-based GUI fetched at configure time.

## Build
Prerequisites:
- CMake 3.8+ (CUDA language enabled), C++17/CUDA17 toolchain
- NVIDIA CUDA Toolkit 11.8+
- NVIDIA OptiX 7.4+ (required by optixBaseLib)
- Git (to fetch cuBQL and ImGUIAX via FetchContent)

Configure/build example:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="D:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake"
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
The GUI opens a file-dialog based workflow. Source mesh, target mesh, output base path, execution mode (`CPU`, `GPU`, `CPU + GPU`), sigma mode, and color map are selected in the panel. Running from the GUI writes suffixed outputs such as `_cpu.obj` and `_gpu.obj` based on the chosen output path.

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
- Dear ImGui + backends (MIT License): see `LICENSE-3rdparty/IMGUI_LICENSE.txt`.
- OptiX and CUDA: subject to NVIDIA EULA/terms, not included here; you must obtain and accept those separately.

If you distribute binaries that bundle cuBQL, include its license notice consistent with Apache 2.0 requirements.

## Dataset
- Large OBJ assets are **not tracked in git** (each >25 MB). Download from the shared folder and place under `dataset/`:
  - `dataset/scan25_cpuCleaned.obj`
  - `dataset/testGPUBinCleaned.obj`
- Download link (Google Drive, view-only/download-only): [here](https://drive.google.com/drive/folders/1zp1rCFbLHC5P_xOCY6sGU30L499t8gZD?usp=sharing)
- Provenance: derived from DTU Dataset scan25 reconstructed via OpenMVS; ensure DTU license compliance for your use.

## GUI Integration
The GUI executable links against `visualizerLib` provided by `ImGUIAX`. The dependency is fetched during CMake configure, so this repository does not need a checked-in copy of `libs/Visualizer`.

## To Do
- Lightweight renderer to preview colored meshes.
- Basic GUI for loading meshes, running deviation on CPU/GPU, and exporting results.
- OptiX-based rendering/acceleration (planned extension; not implemented yet).
