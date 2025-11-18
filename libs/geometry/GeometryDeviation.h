#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "TriangleMesh.h"

namespace SPIN
{
    enum class ExecTag
    {
        HOST,
        DEVICE
    };

    class ColorMapLibrary{
    public:
        static std::vector<float3> JetColorMap(int divCount = 256){
            std::vector<float3> jetMap(divCount);
            for (int i = 0; i < divCount; ++i) {
                float r = std::clamp(1.5f - std::abs(4.0f * (i / float(divCount) - 0.75f)), 0.0f, 1.0f);
                float g = std::clamp(1.5f - std::abs(4.0f * (i / float(divCount) - 0.5f)), 0.0f, 1.0f);
                float b = std::clamp(1.5f - std::abs(4.0f * (i / float(divCount) - 0.25f)), 0.0f, 1.0f);
                jetMap[i] = make_float3(r, g, b);
            }
            return jetMap;
        }
        static std::vector<float3> HotColorMap(int divCount = 256){
            const int n = std::max(2, divCount);
            std::vector<float3> hotMap(n);
            for (int i = 0; i < n; ++i) {
                float t = i / float(n - 1);
                float r = std::clamp(3.0f * t, 0.0f, 1.0f);
                float g = std::clamp(3.0f * t - 1.0f, 0.0f, 1.0f);
                float b = std::clamp(3.0f * t - 2.0f, 0.0f, 1.0f);
                hotMap[i] = make_float3(r, g, b);
            }
            return hotMap;
        }
        static std::vector<float3> CoolColorMap(int divCount = 256){
            const int n = std::max(2, divCount);
            std::vector<float3> coolMap(n);
            for (int i = 0; i < n; ++i) {
                float t = i / float(n - 1);
                float r = t;
                float g = 1.0f - t;
                float b = 1.0f;
                coolMap[i] = make_float3(r, g, b);
            }
            return coolMap;
        }
        static std::vector<float3> TurboColorMap(int divCount = 256){
            const int n = std::max(2, divCount);
            std::vector<float3> turboMap(n);
            for (int i = 0; i < n; ++i) {
                float t  = i / float(n - 1);
                float t2 = t * t;
                float t3 = t2 * t;
                float t4 = t3 * t;
                float t5 = t4 * t;
                // 5th-order polynomial fit of Google Turbo (no blacks/whites at ends)
                float r = 0.1357f + 4.6154f * t - 42.6603f * t2 + 132.1311f * t3 - 152.9424f * t4 + 59.2864f * t5;
                float g = 0.0917f + 2.1946f * t + 4.8429f * t2 - 14.1850f * t3 + 4.2773f * t4 + 2.8295f * t5;
                float b = 0.1067f + 12.6419f * t - 60.5821f * t2 + 145.9810f * t3 - 131.2412f * t4 + 41.5549f * t5;
                turboMap[i] = make_float3(std::clamp(r, 0.0f, 1.0f),
                                          std::clamp(g, 0.0f, 1.0f),
                                          std::clamp(b, 0.0f, 1.0f));
            }
            return turboMap;
        }
        static std::vector<float3> ViridisColorMap(int divCount = 256){
            const int n = std::max(2, divCount);
            std::vector<float3> viridisMap(n);
            for (int i = 0; i < n; ++i) {
                float t = i / float(n - 1);
                float r = std::clamp(0.280268f + 0.165560f * t + 0.476484f * t * t - 0.813533f * t * t * t, 0.0f, 1.0f);
                float g = std::clamp(0.165560f + 0.476484f * t + 0.813533f * t * t - 0.280268f * t * t * t, 0.0f, 1.0f);
                float b = std::clamp(0.476484f + 0.813533f * t + 0.280268f * t * t - 0.165560f * t * t * t, 0.0f, 1.0f);
                viridisMap[i] = make_float3(r, g, b);
            }
            return viridisMap;
        }
        static std::vector<float3> InfernoColorMap(int divCount = 256){
            const int n = std::max(2, divCount);
            std::vector<float3> infernoMap(n);
            for (int i = 0; i < n; ++i) {
                float t = i / float(n - 1);
                float r = std::clamp(0.000218f + 0.106513f * t + 2.224347f * t * t - 5.077576f * t * t * t + 4.493337f * t * t * t * t, 0.0f, 1.0f);
                float g = std::clamp(0.000217f + 0.106514f * t + 2.224348f * t * t - 5.077577f * t * t * t + 4.493338f * t * t * t * t, 0.0f, 1.0f);
                float b = std::clamp(0.000215f + 0.106515f * t + 2.224349f * t * t - 5.077578f * t * t * t + 4.493339f * t * t * t * t, 0.0f, 1.0f);
                infernoMap[i] = make_float3(r, g, b);
            }
            return infernoMap;
        }
        static std::vector<float3> GrayColorMap(int divCount = 256){
            const int n = std::max(2, divCount);
            std::vector<float3> grayMap(n);
            for (int i = 0; i < n; ++i) {
                float v = i / float(n - 1);
                grayMap[i] = make_float3(v, v, v);
            }
            return grayMap;
        }
    };
}

class GeometryDeviationBase
{
protected:
    mutable std::vector<float> deviations;
    TriangleMesh sourceMesh;
    TriangleMesh targetMesh;
    bool uSampling = false;

public:
    GeometryDeviationBase(const TriangleMesh &source, const TriangleMesh &target, bool useSampling = false)
        : sourceMesh(source), targetMesh(target), uSampling(useSampling)
    {
    }
    static float3 deviation2Color(const float &d, int divCount = 4, const std::vector<float3> &colorMap = {})
    {
        // Fallback to the legacy piecewise map: blue -> cyan -> green -> yellow -> red
        static const std::vector<float3> kDefaultMap = {
            make_float3(0, 0, 1),
            make_float3(0, 1, 1),
            make_float3(0, 1, 0),
            make_float3(1, 1, 0),
            make_float3(1, 0, 0)};
        const auto &map = colorMap.empty() ? kDefaultMap : colorMap;

        if (d < 0.0f)
            return colorMap.empty() ? make_float3(0, 0, 0) : map.front();

        // Clamp normalized deviation
        float nd = std::clamp(d, 0.0f, 1.0f);

        // Need at least two colors to interpolate; otherwise return the sole entry
        if (map.size() == 1)
            return map.front();

        // Use divCount to define how many segments we quantize into, regardless of map resolution.
        const int segments = (divCount > 0) ? divCount : static_cast<int>(map.size()) - 1;
        const int safeSegments = std::max(1, segments);

        // Quantize ND to the requested number of segments
        const float step = 1.0f / static_cast<float>(safeSegments);
        nd = std::roundf(nd / step) * step;
        nd = std::clamp(nd, 0.0f, 1.0f);

        // Map quantized segment positions onto the full color map
        float posSeg = nd * static_cast<float>(safeSegments);
        int segIdx = static_cast<int>(posSeg);
        if (segIdx >= safeSegments)
            segIdx = safeSegments - 1;
        float localT = posSeg - static_cast<float>(segIdx);

        auto idxFromSeg = [&](int seg) -> int {
            float ratio = static_cast<float>(seg) / static_cast<float>(safeSegments);
            int mi = static_cast<int>(std::lround(ratio * static_cast<float>(map.size() - 1)));
            return std::clamp(mi, 0, static_cast<int>(map.size() - 1));
        };

        int idx0 = idxFromSeg(segIdx);
        int idx1 = idxFromSeg(segIdx + 1);

        const float3 c0 = map[idx0];
        const float3 c1 = map[idx1];
        // Linear interpolation between map samples derived from the segment mapping
        return make_float3(
            c0.x + (c1.x - c0.x) * localT,
            c0.y + (c1.y - c0.y) * localT,
            c0.z + (c1.z - c0.z) * localT);
    }
    virtual void computeDeviation() const = 0;
    void setDeviation(const std::vector<float> &dev) const { deviations = dev; }
    virtual const std::vector<float> &getDeviations() const = 0;
};

template <SPIN::ExecTag ExecTag>
class GeometryDeviation;

template <>
class GeometryDeviation<SPIN::ExecTag::HOST> : public GeometryDeviationBase
{
public:
    using GeometryDeviationBase::GeometryDeviationBase;

    void computeDeviation() const override;
    const std::vector<float> &getDeviations() const override;
};

template <>
class GeometryDeviation<SPIN::ExecTag::DEVICE> : public GeometryDeviationBase
{
public:
    using GeometryDeviationBase::GeometryDeviationBase;

    void computeDeviation() const override;
    const std::vector<float> &getDeviations() const override;
};
