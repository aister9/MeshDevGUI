MeshDevGUI - Windows x64 portable release

Extract the entire ZIP into a writable folder, then run MeshDevGUI.exe.
Keep the DLL files beside the EXE. Visual Studio, CMake and the CUDA Toolkit
are not needed to run this package. No installer or administrator access is needed.

Requirements and validation
- 64-bit Windows with OpenGL support and an NVIDIA GPU/driver supporting
  this CUDA 13.5 / OptiX build. The NVIDIA driver is not included.
- Tested on Windows with an RTX 2070 SUPER and NVIDIA driver 621.24.
- Compiled with CUDA architecture 75. Other GPUs and clean Windows systems
  have not been validated. AMD/Intel-only PCs cannot render with this backend.
- This release is not code-signed.

Use
1. Mesh List: Open OBJ files..., then select a mesh to view it.
2. Assign Source and Target, then Calculate Deviation in the Deviation panel.
3. Material: enable Show deviation colors for the selected color map, or
   disable it for Base color. Save OBJ exports the colored mesh explicitly.
4. Scene View: left drag rotates, right/middle drag pans, wheel zooms.
   Model changes keep the camera. Fit frames the selected model.
5. Save PNG... lets you choose a folder. Quick Save PNG creates output next
   to the original OBJ. Names use model-BaseColor-001.png or
   model-Deviation-001.png, continuing after the highest existing number.

No sample datasets are bundled. Open your own OBJ files.
Workspace meshes/results are kept in memory and are discarded on exit.

If rendering fails, check/update the NVIDIA driver for CUDA 13.5 support.
Report the error text and BUILD-INFO.json at:
https://github.com/aister9/MeshDevGUI/issues

Source: https://github.com/aister9/MeshDevGUI
Licenses and third-party notices are included in this folder and licenses/.
