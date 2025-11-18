#include "GeometryDeviation.h"

#include "cuBQL/bvh.h"
#include "cuBQL/builder/cpu.h"
#include "cuBQL/queries/pointData/findClosest.h"
#include "cuBQL/queries/triangleData/closestPointOnAnyTriangle.h"

void GeometryDeviation<SPIN::ExecTag::HOST>::computeDeviation() const
{
    if (sourceMesh.vertex.empty() || sourceMesh.index.empty() || targetMesh.vertex.empty())
        return;

    std::vector<cuBQL::box3f> boxes(sourceMesh.index.size());
    std::vector<cuBQL::Triangle> triangles(sourceMesh.index.size());

    for (int i = 0; i < sourceMesh.index.size(); i++)
    {
        uint3 idx = sourceMesh.index[i];
        float3 v0 = sourceMesh.vertex[idx.x];
        float3 v1 = sourceMesh.vertex[idx.y];
        float3 v2 = sourceMesh.vertex[idx.z];

        triangles[i] = cuBQL::Triangle{cuBQL::vec3f{v0.x, v0.y, v0.z},
                                       cuBQL::vec3f{v1.x, v1.y, v1.z},
                                       cuBQL::vec3f{v2.x, v2.y, v2.z}};

        boxes[i] = triangles[i].bounds();
    }

    cuBQL::bvh3f triangleBVH;
    cuBQL::cpuBuilder(triangleBVH, boxes.data(), boxes.size(), cuBQL::BuildConfig());

    std::vector<float> devs;
    devs.reserve(targetMesh.vertex.size());

    for (auto &vt : targetMesh.vertex)
    {
        cuBQL::vec3f queryPoint = {vt.x, vt.y, vt.z};

        cuBQL::triangles::CPAT cpat;
        cpat.runQuery(triangles.data(), triangleBVH, queryPoint);

        devs.push_back(sqrtf(cpat.sqrDist));
    }
    setDeviation(devs);
}

const std::vector<float> &GeometryDeviation<SPIN::ExecTag::HOST>::getDeviations() const
{
    return deviations;
}