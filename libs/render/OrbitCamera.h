#pragma once

#include "3rdParty/helper_math.h"

namespace MeshDev
{
struct CameraFrame
{
    float3 position;
    float3 direction00; // Ray direction at the top-left image corner.
    float3 horizontal;
    float3 vertical;
};

// Uses the project's shared vector math; independent of OWL and the window system.
class OrbitCamera
{
public:
    void fit(const float3 &minimum, const float3 &maximum, float aspect = 1.0f);
    void orbit(float dx, float dy);
    void pan(float dx, float dy, float aspect);
    void zoom(float steps);
    CameraFrame frame(float aspect) const;

private:
    float3 m_center{0, 0, 0};
    float m_radius = 1.0f;
    float m_distance = 3.0f;
    float m_yaw = 0.6f;
    float m_pitch = 0.3f;
};
}
