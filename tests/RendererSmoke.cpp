#include "render/MeshRenderer.h"
#include <iostream>
#include <stdexcept>

using namespace MeshDev;
namespace
{
void require(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }
void checkImage(const RenderedFrame &frame, int width, int height, bool red)
{
    require(frame.rgba && frame.width == width && frame.height == height,"Invalid framebuffer");
    const auto center = frame.rgba[(height/2)*width+width/2];
    const int r = center & 255, g = (center >> 8) & 255, b = (center >> 16) & 255;
    require(red ? (r > 240 && g < 10 && b < 10) : (g > 240 && r < 10 && b < 10),
            "Closest-hit color / scene replacement failed");
    require(frame.rgba[0] != center,"Miss program did not produce the background");
    require((center >> 24) == 255,"Framebuffer alpha is not opaque");
}
}

int main()
{
    try
    {
        TriangleMesh mesh;
        mesh.vertex = {make_float3(-1,-1,0),make_float3(1,-1,0),make_float3(0,1,0)};
        mesh.index = {make_uint3(0,1,2)};
        const CameraFrame camera{{0,0,2},{-1,1,-1},{2,0,0},{0,-2,0}};
        for (int lifetime = 0; lifetime < 2; ++lifetime)
        {
            auto renderer = createOwlRenderer();
            renderer->setMesh(mesh,std::vector<float3>(3,make_float3(1,0,0)));
            checkImage(renderer->render(camera,64,64,false),64,64,true);
            checkImage(renderer->render(camera,96,48,false),96,48,true);
            renderer->setColors(std::vector<float3>(3,make_float3(0,1,0)));
            checkImage(renderer->render(camera,64,64,false),64,64,false);
            bool badColors = false;
            try { renderer->setColors({make_float3(1,0,0)}); }
            catch (const std::invalid_argument &) { badColors = true; }
            require(badColors,"Invalid material update was accepted");
            checkImage(renderer->render(camera,64,64,false),64,64,false);
            renderer->setColors(std::vector<float3>(3,make_float3(1,0,0)));
            TriangleMesh invalid = mesh;
            invalid.index[0].z = 99;
            bool rejected = false;
            try { renderer->setMesh(invalid,std::vector<float3>(3,make_float3(1,0,0))); }
            catch (const std::invalid_argument &) { rejected = true; }
            require(rejected,"Invalid triangle index was accepted");
            checkImage(renderer->render(camera,64,64,false),64,64,true);
            renderer->setMesh(mesh,std::vector<float3>(3,make_float3(0,1,0)));
            checkImage(renderer->render(camera,64,64,false),64,64,false);
            renderer->setMesh(mesh,{make_float3(1,0,0),make_float3(0,1,0),make_float3(0,0,1)});
            const auto interpolated = renderer->render(camera,64,64,false);
            const auto center = interpolated.rgba[32*64+32];
            const int r = center & 255, g = (center >> 8) & 255, b = (center >> 16) & 255;
            require(r > 50 && r < 80 && g > 50 && g < 80 && b > 110 && b < 140,
                    "Barycentric vertex color interpolation failed");
        }
        std::cout << "OWL render, resize, replacement, validation and lifecycle checks passed\n";
        return 0;
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
