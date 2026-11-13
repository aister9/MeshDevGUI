#include "GeometryDeviation.h"
#include <cmath>
#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        TriangleMesh source, target;
        source.vertex = {make_float3(-1,-1,0), make_float3(1,-1,0),
                         make_float3(1,1,0), make_float3(-1,1,0)};
        source.index = {make_uint3(0,1,2), make_uint3(0,2,3)};
        // Closest points on a face, edge, vertex, and the surface itself.
        target.vertex = {make_float3(0,0,2), make_float3(.25f,.5f,-.75f),
                         make_float3(2,0,0), make_float3(2,2,1), make_float3(0,0,0)};
        const float expected[] = {2, .75f, 1, std::sqrt(3.0f), 0};
        GeometryDeviation<SPIN::ExecTag::HOST> cpu(source, target);
        GeometryDeviation<SPIN::ExecTag::DEVICE> gpu(source, target);
        for (int repeat = 0; repeat < 2; ++repeat)
        {
            cpu.computeDeviation();
            gpu.computeDeviation();
            const auto &host = cpu.getDeviations();
            const auto &device = gpu.getDeviations();
            if (host.size() != target.vertex.size() || device.size() != host.size())
                throw std::runtime_error("Deviation result size mismatch");
            for (size_t i = 0; i < host.size(); ++i)
                if (!std::isfinite(host[i]) || !std::isfinite(device[i]) ||
                    std::abs(host[i] - expected[i]) > 1e-5f ||
                    std::abs(device[i] - expected[i]) > 1e-5f)
                    throw std::runtime_error("CPU/GPU deviation differs from analytic distance");
        }
        std::cout << "cuBQL CPU/GPU analytic distance and repeat checks passed\n";
        return 0;
    }
    catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
}
