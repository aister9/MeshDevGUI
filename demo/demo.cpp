#include <iostream>
#include <filesystem>
#include "3rdParty/IO.h"
#include "3rdParty/TimeChecker.h"

#include "Object_t.h"

#include "GeometryDeviation.h"

void writeDeviationPLY(
    const TriangleMesh& mesh,
    const std::vector<float>& deviations,
    const std::string& filename
)
{
    if (mesh.vertex.empty() || mesh.index.empty()) {
        std::cerr << "writeDeviationPLY: empty mesh\n";
        return;
    }
    if (deviations.size() != mesh.vertex.size()) {
        std::cerr << "writeDeviationPLY: deviations.size()("
                  << deviations.size() << ") != vertex.size()("
                  << mesh.vertex.size() << ")\n";
        return;
    }


    std::vector<float3> colors(mesh.vertex.size());
    for (size_t i = 0; i < mesh.vertex.size(); ++i) {
        float nd = deviations[i];  // 0~1
        nd = clamp(nd, 0.0f, 1.0f);
        colors[i] = GeometryDeviationBase::deviation2Color(nd);
    }

    // 2) 파일 열기
    std::ofstream plyOut(filename, std::ios::binary);
    if (!plyOut) {
        std::cerr << "writeDeviationPLY: cannot open file: " << filename << "\n";
        return;
    }

    // 3) PLY 헤더 (binary_little_endian, vertex color + triangle faces)
    plyOut << "ply\n";
    plyOut << "format binary_little_endian 1.0\n";
    plyOut << "element vertex " << mesh.vertex.size() << "\n";
    plyOut << "property float x\n";
    plyOut << "property float y\n";
    plyOut << "property float z\n";
    plyOut << "property uchar red\n";
    plyOut << "property uchar green\n";
    plyOut << "property uchar blue\n";
    plyOut << "element face " << mesh.index.size() << "\n";
    plyOut << "property list uchar int vertex_indices\n";
    plyOut << "end_header\n";

    // 4) vertex + color 쓰기
    for (size_t i = 0; i < mesh.vertex.size(); ++i) {
        const float3& v = mesh.vertex[i];
        const float3& c = colors[i];

        // 위치
        plyOut.write(reinterpret_cast<const char*>(&v.x), sizeof(float));
        plyOut.write(reinterpret_cast<const char*>(&v.y), sizeof(float));
        plyOut.write(reinterpret_cast<const char*>(&v.z), sizeof(float));

        // 컬러 (0~255, clamp)
        auto toUChar = [](float x) -> unsigned char {
            float v = x * 255.0f;
            if (v < 0.0f)   v = 0.0f;
            if (v > 255.0f) v = 255.0f;
            return static_cast<unsigned char>(v);
        };

        unsigned char r = toUChar(c.x);
        unsigned char g = toUChar(c.y);
        unsigned char b = toUChar(c.z);

        plyOut.write(reinterpret_cast<const char*>(&r), sizeof(unsigned char));
        plyOut.write(reinterpret_cast<const char*>(&g), sizeof(unsigned char));
        plyOut.write(reinterpret_cast<const char*>(&b), sizeof(unsigned char));
    }

    // 5) face 쓰기 (항상 3각형, int32 little-endian)
    for (size_t i = 0; i < mesh.index.size(); ++i) {
        const uint3& idx = mesh.index[i];

        // list 길이(항상 3)
        unsigned char vertexCount = 3;
        plyOut.write(reinterpret_cast<const char*>(&vertexCount), 1);

        // PLY는 'int' = 32bit little-endian 고정
        int32_t id[3] = {
            static_cast<int32_t>(idx.x),
            static_cast<int32_t>(idx.y),
            static_cast<int32_t>(idx.z)
        };
        plyOut.write(reinterpret_cast<const char*>(id), sizeof(id)); // 12바이트
    }

    plyOut.close();
}

float computeMedianEdgeLength(const TriangleMesh& mesh)
{
    std::vector<float> edges;
    edges.reserve(mesh.index.size() * 3);

    for (const uint3& f : mesh.index)
    {
        const float3 &v0 = mesh.vertex[f.x];
        const float3 &v1 = mesh.vertex[f.y];
        const float3 &v2 = mesh.vertex[f.z];

        float e0 = length(v1 - v0);
        float e1 = length(v2 - v1);
        float e2 = length(v0 - v2);

        edges.push_back(e0);
        edges.push_back(e1);
        edges.push_back(e2);
    }

    if (edges.empty())
        return 1.0f;

    std::sort(edges.begin(), edges.end());
    return edges[edges.size() / 2];        // median
}

bool compareTwoVector(const std::vector<float>& a, const std::vector<float>& b, float tol = 1e-6f)
{
    if (a.size() != b.size())
        return false;
    for (size_t i = 0; i < a.size(); ++i)
    {
        if (std::abs(a[i] - b[i]) > tol)
            return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    // Resolve defaults relative to the executable location so running from output/Release works.
    std::filesystem::path exeDir = std::filesystem::absolute(argv[0]).parent_path();
    std::filesystem::path projectRoot = exeDir.parent_path().parent_path(); // output/Release -> project root
    std::filesystem::path defaultSource = projectRoot / "dataset/scan25_cpuCleaned.obj";
    std::filesystem::path defaultTarget = projectRoot / "dataset/testGPUBinCleaned.obj";
    std::filesystem::path defaultOutput = projectRoot / "dataset/deviation_output.ply";

    std::string sourcePath = defaultSource.string();
    std::string targetPath = defaultTarget.string();
    std::string outputPath = defaultOutput.string();

    if (argc >= 4)
    {
        sourcePath = argv[1];
        targetPath = argv[2];
        outputPath = argv[3];
    }
    else
    {
        std::cout << "Usage: MeshDevGUI <source.obj/ply> <target.obj/ply> <output_ply>\n";
        std::cout << "Falling back to default dataset paths.\n";
    }

    std::filesystem::path outPlyPath(outputPath);
    if (!outPlyPath.has_extension())
        outPlyPath.replace_extension(".ply");
    std::filesystem::path outObjPath = outPlyPath;
    outObjPath.replace_extension(".obj");

    Object_t objA(sourcePath);
    Object_t objB(targetPath);

    GeometryDeviation<SPIN::ExecTag::HOST> geomDev(*objA.model->meshes[0], *objB.model->meshes[0]);
    const double cpuComputeMs = SPIN::TimeCheck([&](){ geomDev.computeDeviation(); });
    const auto& deviations = geomDev.getDeviations();
    
    GeometryDeviation<SPIN::ExecTag::DEVICE> geomDevDevice(*objA.model->meshes[0], *objB.model->meshes[0]);
    const double gpuComputeMs = SPIN::TimeCheck([&](){ geomDevDevice.computeDeviation(); });
    const auto& deviationsDevice = geomDevDevice.getDeviations();

    std::cout << "CPU deviation compute time: " << cpuComputeMs << " ms\n";
    std::cout << "GPU deviation compute time: " << gpuComputeMs << " ms\n";

    if (compareTwoVector(deviations, deviationsDevice))
    {
        std::cout << "Host and Device deviations match!" << std::endl;
    }
    else
    {
        std::cout << "Host and Device deviations DO NOT match!" << std::endl;
    }
    
    float sigma = computeMedianEdgeLength(*objA.model->meshes[0]);
    std::cout << "Median edge length of source mesh: " << sigma << std::endl;

    //compute deviations/sigma
    for (auto& d : const_cast<std::vector<float>&>(deviations)) {
        d /= sigma;
    }

    writeDeviationPLY(*objB.model->meshes[0], deviations, outPlyPath.string());

    {
        // objB의 첫 번째 mesh를 그대로 쓰는게 목적이면, 굳이 복사 안 하고 참조로 써도 됨
        TriangleMesh &outputMesh = *objB.model->meshes[0];

        // deviations.size() == outputMesh.vertex.size() 라고 가정
        // 혹시 모를 버그 방지를 위해 assert를 넣어도 좋음
        // assert(deviations.size() == outputMesh.vertex.size());

        std::vector<float3> colors(outputMesh.vertex.size());
        for (size_t i = 0; i < outputMesh.vertex.size(); ++i)
        {
            float d = deviations[i];
            float nd = d; // 보통 0~1 근처로 오도록 조정
            colors[i] = GeometryDeviationBase::deviation2Color(nd);
        }

        std::ofstream out(outObjPath.string());
        if (!out)
        {
            std::cerr << "writeDeviationOBJ: cannot open file: " << outObjPath << "\n";
        }

        out << std::fixed << std::setprecision(6);

        // 2) vertex + color (v x y z r g b, color은 0~1 float)
        for (size_t i = 0; i < outputMesh.vertex.size(); ++i)
        {
            const float3 &v = outputMesh.vertex[i];
            const float3 &c = colors[i];

            out << "v "
                << v.x << " " << v.y << " " << v.z << " "
                << c.x << " " << c.y << " " << c.z << "\n";
        }

        // 3) face (OBJ는 1-based index!)
        for (size_t i = 0; i < outputMesh.index.size(); ++i)
        {
            const uint3 &idx = outputMesh.index[i];
            out << "f "
                << (idx.x + 1) << " "
                << (idx.y + 1) << " "
                << (idx.z + 1) << "\n";
        }

        out.close();
    }

    std::cout << "Done." << std::endl;
    return -1;
}
