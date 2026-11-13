#include "viewer/FrameImage.h"
#include "3rdParty/stb_image_write.h"
#include <fstream>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <charconv>
#include <iomanip>
#include <sstream>

namespace MeshDev
{
namespace
{
std::string imagePrefix(const MeshEntry &entry)
{
    std::string name = entry.inputPath.empty() ? entry.name : entry.inputPath.stem().u8string();
    // Preserve the individual mesh number for OBJ files containing multiple meshes.
    const auto fileLabel = entry.inputPath.filename().u8string() + " / ";
    if (!entry.inputPath.empty() && entry.name.compare(0, fileLabel.size(), fileLabel) == 0)
        name += "-mesh-" + entry.name.substr(fileLabel.size());
    for (char &c : name)
        if (static_cast<unsigned char>(c) < 32 || std::string("<>:\"/\\|?*").find(c) != std::string::npos)
            c = '_';
    while (!name.empty() && (name.back() == '.' || name.back() == ' ')) name.pop_back();
    if (name.empty()) name = "mesh";
    return name + (entry.result() && entry.material.showDeviation ? "-Deviation-" : "-BaseColor-");
}
std::string asciiLower(std::string value)
{
    for (char &c : value) if (c >= 'A' && c <= 'Z') c += 'a'-'A';
    return value;
}
}

std::filesystem::path quickImageDirectory(const MeshEntry &entry)
{
    if (entry.inputPath.empty()) throw std::invalid_argument("This mesh has no source folder. Use Save PNG instead.");
    return std::filesystem::absolute(entry.inputPath).parent_path() / "output";
}

std::filesystem::path nextFrameImagePath(const std::filesystem::path &directory, const MeshEntry &entry)
{
    if (directory.empty()) throw std::invalid_argument("Select an image folder");
    const auto folder = std::filesystem::absolute(directory).lexically_normal();
    const auto prefix = imagePrefix(entry);
    const auto lowerPrefix = asciiLower(prefix);
    std::uint64_t maximum = 0;
    if (std::filesystem::exists(folder))
    {
        for (const auto &file : std::filesystem::directory_iterator(folder))
        {
            const auto name = asciiLower(file.path().filename().u8string());
            if (name.size() <= prefix.size()+4 || name.compare(0, prefix.size(), lowerPrefix) != 0 ||
                name.compare(name.size()-4, 4, ".png") != 0) continue;
            const char *first = name.data()+prefix.size(), *last = name.data()+name.size()-4;
            std::uint64_t number = 0;
            const auto parsed = std::from_chars(first, last, number);
            if (parsed.ec == std::errc::result_out_of_range)
                throw std::runtime_error("Image number exceeds the supported range");
            if (parsed.ec == std::errc() && parsed.ptr == last) maximum = std::max(maximum, number);
        }
    }
    if (maximum == std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("No image numbers available");
    std::ostringstream filename;
    filename << prefix << std::setfill('0') << std::setw(3) << maximum+1 << ".png";
    return folder / std::filesystem::u8path(filename.str());
}

std::filesystem::path saveNumberedFramePng(const RenderedFrame &frame,
    const std::filesystem::path &directory, const MeshEntry &entry)
{
    if (directory.empty()) throw std::invalid_argument("Select an image folder");
    std::filesystem::create_directories(directory);
    const auto path = nextFrameImagePath(directory, entry);
    saveFramePng(frame, path);
    return path;
}

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
