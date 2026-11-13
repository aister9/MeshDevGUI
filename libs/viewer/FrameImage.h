#pragma once

#include "render/MeshRenderer.h"
#include <filesystem>

namespace MeshDev
{
// Save the top-to-bottom RGBA framebuffer, without UI overlays or resampling.
void saveFramePng(const RenderedFrame &frame, const std::filesystem::path &path);
}
