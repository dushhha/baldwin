#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace baldwin
{

struct Vertex
{
    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};

struct Mesh
{
    std::string uuid;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

} // namespace baldwin
