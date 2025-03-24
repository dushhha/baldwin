#pragma once

#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "vulkan_device.hpp"

namespace baldwin
{
namespace vk
{

class VulkanSwapchain
{
  public:
    VulkanSwapchain(VulkanDevice& device, int width, int height);
    ~VulkanSwapchain();

    VulkanSwapchain(const VulkanSwapchain& swapchain) = delete;
    VulkanSwapchain& operator=(const VulkanSwapchain& swapchain) = delete;

    VkSwapchainKHR handle() { return _swapchain; };
    VkFormat format() { return _format; };
    VkExtent2D extent() { return _extent; };
    VkImage image(int i) { return _images[i]; };
    VkImageView imageView(int i) { return _imageViews[i]; };

    void resizeSwapchain(int width, int height);

  private:
    void createSwapchain(int width, int height);
    void destroySwapchain();

    VulkanDevice& _device;
    VkSwapchainKHR _swapchain = VK_NULL_HANDLE;
    VkFormat _format = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> _images;
    std::vector<VkImageView> _imageViews;
    VkExtent2D _extent = { 0, 0 };
    bool _sane = false;
};

} // namespace vk
} // namespace baldwin
