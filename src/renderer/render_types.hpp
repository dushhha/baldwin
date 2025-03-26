#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

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

struct SceneData
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewproj;
    glm::vec4 ambientColor; // w for intensity
    glm::vec4 sunlightDirection;
    glm::vec4 sunlightColor; // w for intensity
};

} // namespace baldwin
