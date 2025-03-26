#include "shaders.hpp"

#include <fstream>
#include <format>
#include <iostream>

namespace baldwin
{

std::vector<char> readShaderFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    // TODO: Add proper error handling
    if (!file.is_open())
    {
        throw std::runtime_error(
          std::format("Failed to open file {}", filename));
    }

    std::streamsize fileSize = file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}
} // namespace baldwin
