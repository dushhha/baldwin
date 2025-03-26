#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "vulkan_types.hpp"

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

    Image createImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
                      bool mipmapped = false);
    void destroyImage(Image& image);
    Buffer createBuffer(size_t size, VkBufferUsageFlags usageFlags,
                        VmaMemoryUsage memoryUsage);
    void destroyBuffer(Buffer& buffer);
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void destroyShaderModule(VkShaderModule& module);

    void immediateSubmit(std::function<void(VkCommandBuffer cmd)>&& function);

  private:
    void initImmediate();
    VkInstance _instance = VK_NULL_HANDLE;
    VkPhysicalDevice _gpu = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;
    QueueFamilies _queueFamilies = {};
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    VmaAllocator _allocator = VK_NULL_HANDLE;
    VkCommandPool _immediateCommandPool = VK_NULL_HANDLE;
    VkCommandBuffer _immediateCommandBuffer = VK_NULL_HANDLE;
    VkFence _immediateFence = VK_NULL_HANDLE;
};

} // namespace vk
} // namespace baldwin
