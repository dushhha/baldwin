#pragma once

#include <optional>
#include <vector>
#include <memory>
#include <filesystem>

#include "scene/mesh.hpp"

namespace baldwin
{

std::optional<std::vector<std::shared_ptr<Mesh>>> loadGLTFMeshes(
  const std::filesystem::path& filePath);
}
