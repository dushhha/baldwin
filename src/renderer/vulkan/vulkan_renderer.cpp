#include "vulkan_renderer.hpp"
#include <iostream>

namespace baldwin
{
namespace vk
{

VulkanRenderer::VulkanRenderer(GLFWwindow* window, int width, int height,
                               bool tripleBuffering)
{
}

void VulkanRenderer::run(int frame) { std::cout << "VulkanRenderer-run\n"; }

VulkanRenderer::~VulkanRenderer() {}

} // namespace vk
} // namespace baldwin
