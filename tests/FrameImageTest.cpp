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

        MeshDev::MeshEntry entry;
        entry.inputPath = directory / std::filesystem::u8path(u8"모델.obj");
        entry.name = entry.inputPath.filename().u8string();
        const auto quick = MeshDev::quickImageDirectory(entry);
        const auto prefix = entry.inputPath.stem().u8string();
        auto requireName = [&](const auto &image, const std::string &suffix) {
            if (image.filename().u8string() != prefix+suffix)
                throw std::runtime_error("Unexpected numbered image name: " + image.u8string());
        };
        requireName(MeshDev::nextFrameImagePath(quick, entry), "-BaseColor-001.png");
        if (quick != directory / "output" || std::filesystem::exists(quick))
            throw std::runtime_error("Quick-save destination or preview side effects are incorrect");
        std::vector<std::filesystem::path> saved;
        auto save = [&](const auto &folder) {
            saved.push_back(MeshDev::saveNumberedFramePng(frame, folder, entry));
            return saved.back();
        };
        requireName(save(quick), "-BaseColor-001.png");
        requireName(save(quick), "-BaseColor-002.png");
        // A result must actually be displayed before the filename says Deviation.
        entry.results.emplace_back();
        requireName(save(quick), "-Deviation-001.png");
        requireName(save(quick), "-Deviation-002.png");
        entry.material.showDeviation = false;
        requireName(save(quick), "-BaseColor-003.png");
        const auto custom = directory / "chosen folder";
        requireName(save(custom), "-BaseColor-001.png");
        // Continue after the highest existing number, including gaps and uppercase extensions.
        saved.push_back(quick / std::filesystem::u8path(prefix+"-BaseColor-1000.PNG"));
        { std::ofstream existing(saved.back()); existing << "existing image"; }
        requireName(save(quick), "-BaseColor-1001.png");
        if (std::filesystem::file_size(saved[saved.size()-2]) != 14)
            throw std::runtime_error("An existing image was overwritten");
        entry.name += " / 2";
        requireName(MeshDev::nextFrameImagePath(quick, entry), "-mesh-2-BaseColor-001.png");
        entry.inputPath.clear();
        entry.name = "other/model?";
        if (MeshDev::nextFrameImagePath(quick, entry).filename() != "other_model_-BaseColor-001.png")
            throw std::runtime_error("Unsafe model filename was not sanitized");
        failed = false;
        try { MeshDev::quickImageDirectory(entry); }
        catch (const std::invalid_argument &) { failed = true; }
        if (!failed) throw std::runtime_error("A mesh without a source path cannot use quick save");
        failed = false;
        try { MeshDev::saveNumberedFramePng(frame, path, entry); }
        catch (const std::filesystem::filesystem_error &) { failed = true; }
        if (!failed) throw std::runtime_error("A file was accepted as an image directory");
        for (const auto &image : saved) std::filesystem::remove(image);
        std::filesystem::remove(custom);
        std::filesystem::remove(quick);
        std::filesystem::remove(path);
        std::filesystem::remove(directory);
        std::cout << "PNG round-trip, quick/custom folders, per-mode numbering, Unicode paths and failure checks passed\n";
        return 0;
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
