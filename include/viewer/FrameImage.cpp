#include "viewer/FrameImage.h"
#include "3rdParty/stb_image_write.h"
#include <fstream>
#include <limits>
#include <stdexcept>

namespace MeshDev
{
void saveFramePng(const RenderedFrame &frame, const std::filesystem::path &path)
{
    if (!frame.rgba || frame.width <= 0 || frame.height <= 0 ||
        frame.width > std::numeric_limits<int>::max()/4)
        throw std::invalid_argument("Invalid image framebuffer");
    std::ofstream output(path, std::ios::binary);
    if (!output) throw std::runtime_error("Cannot open PNG output: " + path.u8string());
    const auto write = [](void *context, void *data, int size) {
        static_cast<std::ofstream *>(context)->write(static_cast<const char *>(data), size);
    };
    const bool encoded = stbi_write_png_to_func(write, &output, frame.width, frame.height,
                                               4, frame.rgba, frame.width*4) != 0;
    output.close();
    if (!encoded || !output)
        throw std::runtime_error("Failed to write PNG: " + path.u8string());
}
}
