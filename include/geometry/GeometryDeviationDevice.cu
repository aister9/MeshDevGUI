#include "GeometryDeviation.h"
#include "3rdParty/CUDABuffer.h"

#include "cuBQL/bvh.h"
#include "cuBQL/builder/cuda.h"
#include "cuBQL/queries/triangleData/closestPointOnAnyTriangle.h"

#define CUBQL_GPU_BUILDER_IMPLEMENTATION 1

using cuBQL::divRoundUp;

__global__ void computeTrianglesAndBoxes(const float3* vertices, const uint3* indices, cuBQL::Triangle* triangles, cuBQL::box3f* boxes, size_t numTriangles)
{
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numTriangles) return;

    uint3 triIdx = indices[idx];
    float3 v0 = vertices[triIdx.x];
    float3 v1 = vertices[triIdx.y];
    float3 v2 = vertices[triIdx.z];

    triangles[idx] = cuBQL::Triangle{cuBQL::vec3f{v0.x, v0.y, v0.z},
                                    cuBQL::vec3f{v1.x, v1.y, v1.z},
                                    cuBQL::vec3f{v2.x, v2.y, v2.z}};

    boxes[idx] = triangles[idx].bounds();
}

__global__ void runQueries(cuBQL::bvh3f trianglesBVH, const cuBQL::Triangle *triangles, const float3* queryPoints, float* outDeviations, size_t numQueriues){
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= numQueriues) return;

    cuBQL::vec3f queryPoint = {queryPoints[idx].x, queryPoints[idx].y, queryPoints[idx].z};

    cuBQL::triangles::CPAT cpat;
    cpat.runQuery(triangles, trianglesBVH, queryPoint);

    outDeviations[idx] = sqrtf(cpat.sqrDist);
}

void GeometryDeviation<SPIN::ExecTag::DEVICE>::computeDeviation() const
{
    if (sourceMesh.vertex.empty() || sourceMesh.index.empty() || targetMesh.vertex.empty())
        return;

    CUDABuffer d_boxes;
    d_boxes.alloc(sizeof(cuBQL::box3f) * sourceMesh.index.size());
    CUDABuffer d_triangles;
    d_triangles.alloc(sizeof(cuBQL::Triangle) * sourceMesh.index.size());
    CUDABuffer d_vertices;
    d_vertices.alloc_and_upload(sourceMesh.vertex);
    CUDABuffer d_indices;
    d_indices.alloc_and_upload(sourceMesh.index);

    int numTri = sourceMesh.index.size();

    computeTrianglesAndBoxes<<<divRoundUp(numTri, 256), 256>>>(
        (const float3*)d_vertices.d_pointer(),
        (const uint3*)d_indices.d_pointer(),
        (cuBQL::Triangle*)d_triangles.d_pointer(),
        (cuBQL::box3f*)d_boxes.d_pointer(),
        numTri);


    cuBQL::bvh3f triangleBVH;
    cuBQL::gpuBuilder(triangleBVH, (cuBQL::box3f*)d_boxes.d_pointer(), numTri, cuBQL::BuildConfig());

    CUDABuffer d_queryPoints;
    d_queryPoints.alloc_and_upload(targetMesh.vertex);
    CUDABuffer d_deviations;
    d_deviations.alloc(sizeof(float) * targetMesh.vertex.size());
    int numQueries = targetMesh.vertex.size();
    runQueries<<<divRoundUp(numQueries, 256), 256>>>(
        triangleBVH,
        (const cuBQL::Triangle*)d_triangles.d_pointer(),
        (const float3*)d_queryPoints.d_pointer(),
        (float*)d_deviations.d_pointer(),
        numQueries);
    deviations.resize(numQueries);
    d_deviations.download(deviations.data(), numQueries);

    cuBQL::cuda::free(triangleBVH);
    d_boxes.free();
    d_triangles.free();
    d_vertices.free();
    d_indices.free();
    d_queryPoints.free();
    d_deviations.free();
}

const std::vector<float> &GeometryDeviation<SPIN::ExecTag::DEVICE>::getDeviations() const
{
    return deviations;
}