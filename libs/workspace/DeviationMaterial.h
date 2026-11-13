#pragma once
#include "geometry/TriangleMesh.h"

namespace MeshDev
{
enum class ColorMap { Jet, Turbo, Viridis, Hot, Cool, Gray };
enum class SigmaMethod { MedianEdge, MeanEdge };

struct DeviationMaterial
{
    ColorMap colorMap = ColorMap::Jet;
    SigmaMethod sigmaMethod = SigmaMethod::MedianEdge;
    float sigmaScale = 1.0f;
    float3 baseColor = {0.72f, 0.78f, 0.86f};
    bool showDeviation = true;
};

std::vector<float3> colorMapSamples(ColorMap map, int count = 256);
void validateMaterial(const DeviationMaterial &material);
std::vector<float3> encodeDeviation(const std::vector<float> &distances,
                                    const DeviationMaterial &material, float sigma);
}
