#pragma once

#include <cstdint>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace baldwin
{
namespace vk
{

struct QueueFamilies
{
    uint32_t graphics;
    uint32_t present;
};

class VulkanDevice
{
  public:
#ifndef NDEBUG
    const bool _enableValidationLayers = true;
#else
    const bool _enableValidationLayers = false;
#endif

    VulkanDevice(GLFWwindow* window);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;
    VulkanDevice(VulkanDevice&) = delete;
    VulkanDevice& operator=(VulkanDevice&) = delete;

    // Accessors
    VkDevice handle() { return _device; };
    VkPhysicalDevice physicalDevice() { return _gpu; };
    VkSurfaceKHR surface() { return _surface; }
    VkQueue graphicsQueue() { return _graphicsQueue; }
    VkQueue presentQueue() { return _presentQueue; }
    QueueFamilies queueFamilies() { return _queueFamilies; };

  private:
    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _gpu = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;
    QueueFamilies _queueFamilies = {};
};

} // namespace vk
} // namespace baldwin
