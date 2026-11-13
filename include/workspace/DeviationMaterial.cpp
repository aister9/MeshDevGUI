#include "workspace/DeviationMaterial.h"
#include "geometry/GeometryDeviation.h"
#include <stdexcept>

namespace MeshDev
{
std::vector<float3> colorMapSamples(ColorMap map, int count)
{
    using SPIN::ColorMapLibrary;
    switch (map)
    {
    case ColorMap::Jet: return ColorMapLibrary::JetColorMap(count);
    case ColorMap::Turbo: return ColorMapLibrary::TurboColorMap(count);
    case ColorMap::Viridis: return ColorMapLibrary::ViridisColorMap(count);
    case ColorMap::Hot: return ColorMapLibrary::HotColorMap(count);
    case ColorMap::Cool: return ColorMapLibrary::CoolColorMap(count);
    case ColorMap::Gray: return ColorMapLibrary::GrayColorMap(count);
    }
    throw std::invalid_argument("Unknown color map");
}

void validateMaterial(const DeviationMaterial &m)
{
    if (!std::isfinite(m.sigmaScale) || m.sigmaScale <= 0)
        throw std::invalid_argument("Sigma scale must be positive and finite");
    if (!std::isfinite(m.baseColor.x) || !std::isfinite(m.baseColor.y) || !std::isfinite(m.baseColor.z)
        || m.baseColor.x < 0 || m.baseColor.x > 1 || m.baseColor.y < 0 || m.baseColor.y > 1
        || m.baseColor.z < 0 || m.baseColor.z > 1)
        throw std::invalid_argument("Base color must be in [0, 1]");
    if (m.sigmaMethod != SigmaMethod::MedianEdge && m.sigmaMethod != SigmaMethod::MeanEdge)
        throw std::invalid_argument("Unknown sigma method");
    (void)colorMapSamples(m.colorMap, 2);
}

std::vector<float3> encodeDeviation(const std::vector<float> &distances,
                                    const DeviationMaterial &material, float sigma)
{
    validateMaterial(material);
    if (!std::isfinite(sigma) || sigma <= 0)
        throw std::invalid_argument("Mesh must have a positive finite edge length for color mapping");
    const auto palette = colorMapSamples(material.colorMap);
    std::vector<float3> colors;
    colors.reserve(distances.size());
    for (float d : distances)
    {
        if (!std::isfinite(d) || d < 0)
            throw std::invalid_argument("Invalid deviation distance");
        colors.push_back(GeometryDeviationBase::deviation2Color(d / sigma, 256, palette));
    }
    return colors;
}
}
