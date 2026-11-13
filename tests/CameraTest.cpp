#include "render/OrbitCamera.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace MeshDev;
namespace
{
float3 direction(const CameraFrame &f)
{
    return f.direction00 + 0.5f*(f.horizontal + f.vertical);
}
void require(bool condition, const char *message) { if (!condition) throw std::runtime_error(message); }

void checkFit(float aspect)
{
    OrbitCamera camera;
    camera.fit({-2,-3,-4},{2,3,4},aspect);
    const auto f = camera.frame(aspect);
    const auto forward = direction(f);
    require(std::abs(dot(forward,f.horizontal)) < 1e-5f,"Horizontal camera basis is not perpendicular");
    require(std::abs(dot(forward,f.vertical)) < 1e-5f,"Vertical camera basis is not perpendicular");
    for (float x : {-2.0f,2.0f}) for (float y : {-3.0f,3.0f}) for (float z : {-4.0f,4.0f})
    {
        const auto toCorner = make_float3(x,y,z) - f.position;
        const float depth = dot(toCorner,forward);
        require(depth > 0,"Fitted bounds are behind the camera");
        require(std::abs(dot(toCorner,f.horizontal)/dot(f.horizontal,f.horizontal)/depth) < 0.5f,
                "Fitted bounds exceed horizontal field of view");
        require(std::abs(dot(toCorner,f.vertical)/dot(f.vertical,f.vertical)/depth) < 0.5f,
                "Fitted bounds exceed vertical field of view");
    }
    camera.orbit(1000,1000);
    camera.zoom(1000);
    camera.pan(0.1f,-0.2f,aspect);
    const float3 position = camera.frame(aspect).position;
    for (float value : {position.x, position.y, position.z})
        require(std::isfinite(value),"Camera interaction produced a non-finite position");
}
}

int main()
{
    try
    {
        checkFit(1.0f);
        checkFit(0.2f);
        checkFit(4.0f);
        OrbitCamera camera;
        camera.fit({1,1,1},{1,1,1});
        const float3 position = camera.frame(1).position;
        for (float value : {position.x, position.y, position.z})
            require(std::isfinite(value),"Degenerate bounds produced a non-finite camera");
        bool rejected = false;
        try { camera.fit({2,0,0},{1,0,0}); } catch (const std::invalid_argument &) { rejected = true; }
        require(rejected,"Inverted bounds were accepted");
        std::cout << "Camera fit and interaction checks passed\n";
        return 0;
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
