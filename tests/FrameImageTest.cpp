#include "viewer/FrameImage.h"
#include "3rdParty/stb_image.h"
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <vector>

int main()
{
    const auto directory = std::filesystem::temp_directory_path() /
        ("meshdev-image-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try
    {
        std::filesystem::create_directory(directory);
        const auto path = directory / std::filesystem::u8path(u8"현재 시점.png");
        // Different top/bottom rows and distinct RGBA channels expose flips and channel swaps.
        const std::array<std::uint32_t, 6> pixels = {
            0xff0000ff, 0xff00ff00, 0xffff0000, 0xff563412, 0xffabcdef, 0x80402010};
        const MeshDev::RenderedFrame frame{pixels.data(), 2, 3};
        MeshDev::saveFramePng(frame, path);
        std::ifstream input(path, std::ios::binary);
        const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(input)), {});
        input.close();
        int width = 0, height = 0, channels = 0;
        auto *decoded = stbi_load_from_memory(bytes.data(), static_cast<int>(bytes.size()),
                                             &width, &height, &channels, 4);
        const bool matches = decoded && width == 2 && height == 3 &&
            std::memcmp(decoded, pixels.data(), sizeof(pixels)) == 0;
        stbi_image_free(decoded);
        if (!matches) throw std::runtime_error("PNG dimensions, orientation or RGBA pixels changed");
        bool failed = false;
        try { MeshDev::saveFramePng(frame, directory / "missing" / "image.png"); }
        catch (const std::runtime_error &) { failed = true; }
        if (!failed) throw std::runtime_error("Output failure was not reported");
        failed = false;
        try { MeshDev::saveFramePng({}, path); }
        catch (const std::invalid_argument &) { failed = true; }
        if (!failed || std::filesystem::file_size(path) != bytes.size())
            throw std::runtime_error("Invalid frame was not rejected before opening output");
        std::filesystem::remove(path);
        std::filesystem::remove(directory);
        std::cout << "PNG round-trip, Unicode path and failure checks passed\n";
        return 0;
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
