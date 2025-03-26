#include "gltf.hpp"

#include <cstdint>
#include <iostream>
#include <format>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include "utils/uuid.hpp"

namespace baldwin
{

std::optional<std::vector<std::shared_ptr<Mesh>>> loadGLTFMeshes(
  const std::filesystem::path& filePath)
{
    std::cout << std::format("Loading GLTF meshes from : {}\n",
                             filePath.string());

    fastgltf::Parser parser{};
    constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;
    auto data = fastgltf::GltfDataBuffer::FromPath(filePath.string());

    auto gltfFile = fastgltf::MappedGltfFile::FromPath(filePath);
    if (!bool(gltfFile))
    {
        std::cerr << "Failed to open glTF file: "
                  << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
        return {};
    }

    auto asset = parser.loadGltfBinary(
      gltfFile.get(), filePath.parent_path(), gltfOptions);
    if (asset.error() != fastgltf::Error::None)
    {
        std::cerr << "Failed to load glTF: "
                  << fastgltf::getErrorMessage(asset.error()) << '\n';
        return {};
    }

    // We start to iterate over gltf meshes
    std::vector<std::shared_ptr<Mesh>> meshPtrs;
    for (auto& mesh : asset->meshes)
    {
        Mesh newMesh{ .uuid = generateUUID() };

        int initialVtx = 0;
        for (auto& p : mesh.primitives)
        {
            // Access indices
            fastgltf::Accessor&
              indexaccessor = asset->accessors[p.indicesAccessor.value()];
            newMesh.indices.reserve(newMesh.indices.size() +
                                    indexaccessor.count);

            fastgltf::iterateAccessor<std::uint32_t>(
              asset.get(),
              indexaccessor,
              [&](std::uint32_t idx)
              {
                  newMesh.indices.push_back(idx + initialVtx);
              });

            // Access position and create vertex
            fastgltf::Accessor&
              posaccessor = asset->accessors[p.findAttribute("POSITION")
                                               ->accessorIndex];
            newMesh.vertices.reserve(newMesh.vertices.size() +
                                     posaccessor.count);

            fastgltf::iterateAccessorWithIndex<glm::vec3>(
              asset.get(),
              posaccessor,
              [&](glm::vec3 p, size_t index)
              {
                  Vertex newvtx;
                  newvtx.position = p;
                  newvtx.normal = { 1, 0, 0 };
                  newvtx.color = glm::vec4{ 1.f };
                  newvtx.uv_x = 0;
                  newvtx.uv_y = 0;
                  newMesh.vertices.push_back(newvtx);
              });

            // Normal
            auto normals = p.findAttribute("NORMAL");
            if (normals != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec3>(
                  asset.get(),
                  asset->accessors[normals->accessorIndex],
                  [&](glm::vec3 n, size_t index)
                  {
                      newMesh.vertices[initialVtx + index].normal = n;
                  });
            }

            // UV
            auto uvs = p.findAttribute("TEXCOORD_0");
            if (uvs != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec2>(
                  asset.get(),
                  asset->accessors[uvs->accessorIndex],
                  [&](glm::vec2 uv, size_t index)
                  {
                      newMesh.vertices[initialVtx + index].uv_x = uv.x;
                      newMesh.vertices[initialVtx + index].uv_y = uv.y;
                  });
            }

            // Vertex color
            auto colors = p.findAttribute("COLOR_0");
            if (colors != p.attributes.end())
            {
                fastgltf::iterateAccessorWithIndex<glm::vec4>(
                  asset.get(),
                  asset->accessors[colors->accessorIndex],
                  [&](glm::vec4 c, size_t index)
                  {
                      newMesh.vertices[initialVtx + index].color = c;
                  });
            }
            initialVtx += posaccessor.count;
        }

        // NOTE: Used to display vertex normals
        constexpr bool OverrideColors = true;
        if (OverrideColors)
        {
            for (Vertex& vtx : newMesh.vertices)
            {
                vtx.color = glm::vec4(vtx.normal, 1.f);
            }
        }
        /*std::cout << newMesh.indices.size() << std::endl;*/
        meshPtrs.emplace_back(std::make_shared<Mesh>(std::move(newMesh)));
    }

    return meshPtrs;
}

} // namespace baldwin
