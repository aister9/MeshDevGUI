# MeshDevGUI

CPU/GPU geometry deviation measurement and visualization. Uses cuBQL BVH builders/queries to compute per-vertex distances between a source mesh and a target mesh, then writes out colored PLY/OBJ for inspection. Includes simple GLSL renderer scaffolding.

## Features
- CPU and GPU deviation calculator (cuBQL BVH).
- Color mapping with selectable palettes (jet, hot, cool, turbo, viridis, gray).
- Outputs colored PLY (binary) and OBJ.
- Command-line interface for source/target/output paths.

## Build
Prerequisites:
- CMake 3.8+ (CUDA language enabled), C++17/CUDA17 toolchain
- NVIDIA CUDA Toolkit 11.8+
- NVIDIA OptiX 7.4+ (required by optixBaseLib)
- Git (to fetch cuBQL via FetchContent)

Configure/build example:
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="D:/vcpkg/vcpkg/scripts/buildsystems/vcpkg.cmake"
cmake --build build --config Release
```

> Note: cuBQL builds CUDA instantiations; ensure `CMAKE_CUDA_ARCHITECTURES` matches your GPU. DLLs (e.g., `cuBQL_cuda_float3`) are copied next to the executable via a post-build step in `demo/CMakeLists.txt`.

## Run
From `output/Release` (defaults resolve relative to the executable):
```bash
./MeshDevGUI <source.obj/ply> <target.obj/ply> <output_ply>
```
If arguments are omitted, it uses `dataset/scan25_cpuCleaned.obj`, `dataset/testGPUBinCleaned.obj`, and writes `dataset/deviation_output.{ply,obj}` in the project root.

> Note: PLY output is currently experimental; primary supported interchange format is OBJ.

## Repository layout (key files)
- `demo/demo.cpp` — CLI entry; writes colored PLY/OBJ.
- `libs/geometry/GeometryDeviation.h` — deviation API; color maps.
- `include/geometry/GeometryDeviationHost.cpp` — CPU implementation.
- `include/geometry/GeometryDeviationDevice.cu` — GPU stub/impl (requires cuBQL CUDA).
- `libs/geometry/CMakeLists.txt` — geometryLib + CUDA source setup.
- `demo/CMakeLists.txt` — app target and DLL copy rule.

## License
This project is licensed under the Apache License 2.0 (see `LICENSE`).

Third-party:
- cuBQL (Apache 2.0, NVIDIA): fetched via FetchContent; follows its license.
- OptiX and CUDA: subject to NVIDIA EULA/terms, not included here; you must obtain and accept those separately.

If you distribute binaries that bundle cuBQL, include its license notice consistent with Apache 2.0 requirements.

## To Do
- Lightweight renderer to preview colored meshes.
- Basic GUI for loading meshes, running deviation on CPU/GPU, and exporting results.
