#pragma once

#include "render/MeshRenderer.h"
#include "workspace/MeshWorkspace.h"
#include <filesystem>

namespace MeshDev
{
// Save the top-to-bottom RGBA framebuffer, without UI overlays or resampling.
void saveFramePng(const RenderedFrame &frame, const std::filesystem::path &path);
std::filesystem::path quickImageDirectory(const MeshEntry &entry);
std::filesystem::path nextFrameImagePath(const std::filesystem::path &directory, const MeshEntry &entry);
// Create the output folder and select the next number at save time.
std::filesystem::path saveNumberedFramePng(const RenderedFrame &frame,
    const std::filesystem::path &directory, const MeshEntry &entry);
}
