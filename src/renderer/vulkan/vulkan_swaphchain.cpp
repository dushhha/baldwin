#include "vulkan_swapchain.hpp"

#include <iostream>
#include <VkBootstrap.h>
#include <vulkan/vulkan_core.h>

namespace baldwin
{
namespace vk
{

VulkanSwapchain::VulkanSwapchain(VulkanDevice& device, int width, int height)
  : _device{ device }
{
    createSwapchain(width, height);
}

void VulkanSwapchain::createSwapchain(int width, int height)
{
    _format = VK_FORMAT_B8G8R8A8_UNORM;

    vkb::SwapchainBuilder swapchainBuilder{ _device.physicalDevice(),
                                            _device.handle(),
                                            _device.surface() };
    vkb::Swapchain
      vkbSwapchain = swapchainBuilder
                       //.use_default_format_selection()
                       .set_desired_format(VkSurfaceFormatKHR{
                         .format = _format,
                         .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR })
                       // use vsync present mode
                       .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                       .set_desired_extent(width, height)
                       .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                       .build()
                       .value();

    _extent = vkbSwapchain.extent;
    // store swapchain and its related imagessnip-next-choice
    _swapchain = vkbSwapchain.swapchain;
    _images = vkbSwapchain.get_images().value();
    _imageViews = vkbSwapchain.get_image_views().value();
    sane = true;
}

void VulkanSwapchain::destroySwapchain()
{
    for (auto& imgView : _imageViews)
    {
        vkDestroyImageView(_device.handle(), imgView, nullptr);
    }
    vkDestroySwapchainKHR(_device.handle(), _swapchain, nullptr);
}

void VulkanSwapchain::reconstruct(int width, int height)
{
    assert(_device.handle() != VK_NULL_HANDLE);
    assert(_swapchain != VK_NULL_HANDLE &&
           "Resize swapchain called before creation");

    vkDeviceWaitIdle(_device.handle());
    destroySwapchain();
    createSwapchain(width, height);
}

VulkanSwapchain::~VulkanSwapchain()
{
#ifndef NDEBUG
    std::cout << "-- VulkanSwapchain cleanup\n";
#endif
    destroySwapchain();
}

} // namespace vk
} // namespace baldwin
