#include "render/OrbitCamera.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace MeshDev
{
namespace
{
constexpr float halfFovTangent = 0.41421356237f; // 45 degree vertical FOV
float3 right(float yaw) { return make_float3(std::cos(yaw), 0, -std::sin(yaw)); }
float3 up(float yaw, float pitch)
{
    return {-std::sin(yaw)*std::sin(pitch), std::cos(pitch), -std::cos(yaw)*std::sin(pitch)};
}
}

void OrbitCamera::fit(const float3 &minimum, const float3 &maximum, float aspect)
{
    if (!std::isfinite(minimum.x) || !std::isfinite(minimum.y) || !std::isfinite(minimum.z)
        || !std::isfinite(maximum.x) || !std::isfinite(maximum.y) || !std::isfinite(maximum.z)
        || minimum.x > maximum.x || minimum.y > maximum.y || minimum.z > maximum.z)
        throw std::invalid_argument("Invalid mesh bounds");
    m_center = 0.5f*minimum + 0.5f*maximum;
    // Keep the squared extent calculation in double to avoid float overflow on large bounds.
    const double halfX = (double(maximum.x) - minimum.x)*0.5;
    const double halfY = (double(maximum.y) - minimum.y)*0.5;
    const double halfZ = (double(maximum.z) - minimum.z)*0.5;
    const double radiusSquared = halfX*halfX + halfY*halfY + halfZ*halfZ;
    m_radius = std::max(1e-6f, static_cast<float>(std::sqrt(radiusSquared)));
    if (!std::isfinite(m_radius) || m_radius > 1e30f)
        throw std::invalid_argument("Mesh bounds exceed camera range");
    const float tangent = halfFovTangent * std::min(1.0f, std::max(0.01f, aspect));
    m_distance = 1.1f*m_radius*std::sqrt(1.0f+tangent*tangent)/tangent;
    m_yaw = 0.6f;
    m_pitch = 0.3f;
}

void OrbitCamera::orbit(float dx, float dy)
{
    m_yaw = std::remainder(m_yaw - dx*0.01f, 6.28318530718f);
    m_pitch = std::clamp(m_pitch + dy*0.01f, -1.55f, 1.55f);
}

void OrbitCamera::pan(float dx, float dy, float aspect)
{
    const float span = 2.0f*m_distance*halfFovTangent;
    m_center += right(m_yaw)*(-dx*span*aspect) + up(m_yaw,m_pitch)*(dy*span);
}

void OrbitCamera::zoom(float steps)
{
    m_distance = std::clamp(m_distance*std::exp(-std::clamp(steps,-20.0f,20.0f)*0.15f),
                            m_radius*0.02f, m_radius*10000.0f);
}

CameraFrame OrbitCamera::frame(float aspect) const
{
    const float3 outward{std::sin(m_yaw)*std::cos(m_pitch), std::sin(m_pitch), std::cos(m_yaw)*std::cos(m_pitch)};
    CameraFrame result;
    result.position = m_center + outward*m_distance;
    result.horizontal = right(m_yaw)*(2.0f*halfFovTangent*std::max(0.01f,aspect));
    // Image rows run top to bottom, matching ImGui's default UVs.
    result.vertical = up(m_yaw,m_pitch)*(-2.0f*halfFovTangent);
    result.direction00 = outward*(-1.0f) - 0.5f*(result.horizontal + result.vertical);
    return result;
}
}
