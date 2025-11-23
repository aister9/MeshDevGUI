#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/normal.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/string_cast.hpp>
#include <GL/freeglut.h>

#include "3rdParty/helper_math.h"
#include <vector>

namespace SPIN
{
    namespace GLSL
    {
        class Cube{
            private:
                float size;
                std::vector<float3> vertices;
                std::vector<uint3> indices;
            public:
            Cube(float size = 1.0f){
                this->size = size;
                vertices = {
                    { size,  size, -size},
                    { size, -size, -size},
                    { size,  size,  size},
                    { size, -size,  size},
                    {-size,  size, -size},
                    {-size, -size, -size},
                    {-size,  size,  size},
                    {-size, -size,  size}};
                indices = {
                    {5, 3, 1}, {3, 8, 4}, {7, 6, 8}, {2, 8, 6}, {1, 4, 2}, {5, 2, 6},
                    {5, 7, 3}, {3, 7, 8}, {7, 5, 6}, {2, 4, 8}, {1, 3, 4}, {5, 1, 2}};
            }
        };

        static const char *baseShader_vtx =
            "#version 460 core\n"
            "\n"
            "layout(location = 0) in vec3 vertexPosition_modelspace;\n"
            "layout(location = 1) in vec3 vertexColor;\n" // [NEW] vertex color attribute
            "\n"
            "uniform mat4 MVP;\n"
            "\n"
            "out vec3 vColor;                                 // [NEW]\n"
            "\n"
            "void main() {\n"
            "    vColor = vertexColor;\n" // [NEW]\n"
            "    vec4 p = vec4(vertexPosition_modelspace, 1.0);\n"
            "    gl_Position = MVP * p;\n"
            "}\n";
        static const char *baseShader_frag =
            "#version 460 core\n"
            "\n"
            "uniform vec4 mtlColor;       // base 색 (RGBA)\n"
            "uniform vec3 matAmbient;     // [NEW] ambient 계수\n"
            "uniform vec3 matDiffuse;     // [NEW] diffuse 계수\n"
            "uniform bool useVertexColor; // [NEW] vertex color map 사용할지 여부\n"
            "\n"
            "in vec3 vColor;\n"
            "out vec4 color;\n"
            "\n"
            "void main() {\n"
            "    vec3 baseColor = useVertexColor ? vColor : mtlColor.rgb;\n"
            "\n"
            "    vec3 ambientTerm = matAmbient * baseColor;\n"
            "    vec3 diffuseTerm = matDiffuse * baseColor;\n"
            "    vec3 finalRGB = ambientTerm + diffuseTerm;\n"
            "\n"
            "    color = vec4(finalRGB, mtlColor.a);\n"
            "}\n";

    }
}