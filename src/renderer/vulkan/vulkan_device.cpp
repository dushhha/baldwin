#include "vulkan_device.hpp"

#include <iostream>
#include <VkBootstrap.h>

namespace baldwin
{
namespace vk
{

VulkanDevice::VulkanDevice(GLFWwindow* window)
{
    uint32_t extensionCount;
    const char** extensions = glfwGetRequiredInstanceExtensions(
      &extensionCount);
#ifndef NDEBUG
    std::cout << "Required Vulkan Instance Extensions : \n";
    for (int i = 0; i < extensionCount; i++)
    {
        std::cout << extensions[i] << std::endl;
    }
#endif

    // Instance
    vkb::InstanceBuilder builder;
    builder.set_app_name("Baldwin Engine Application")
      .request_validation_layers()
      .use_default_debug_messenger()
      .require_api_version(1, 3, 0);

    if (_enableValidationLayers)
    {
        builder.enable_extensions(extensionCount, extensions);
    }
    vkb::Instance vkbInst = builder.build().value();
    _instance = vkbInst.instance;

    // Surface
    glfwCreateWindowSurface(_instance, window, nullptr, &_surface);

    // Physical and logical devices
    VkPhysicalDeviceVulkan13Features features13 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };
    features13.dynamicRendering = true;
    features13.synchronization2 = true;

    VkPhysicalDeviceVulkan12Features features12 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
    };
    features12.bufferDeviceAddress = true;
    features12.descriptorIndexing = true;

    vkb::PhysicalDeviceSelector selector{ vkbInst };
    vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 3)
                                           .set_required_features_13(features13)
                                           .set_required_features_12(features12)
                                           .set_surface(_surface)
                                           .select()
                                           .value();
    std::cout << "Selected GPU :" << physicalDevice.name << std::endl;
    vkb::DeviceBuilder deviceBuilder{ physicalDevice };
    vkb::Device vkbDevice = deviceBuilder.build().value();

    _device = vkbDevice.device;
    _gpu = vkbDevice.physical_device;

    // Queue and queue family
    _queueFamilies
      .graphics = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();
    _graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
    _queueFamilies.present = vkbDevice.get_queue_index(vkb::QueueType::present)
                               .value();
    _presentQueue = vkbDevice.get_queue(vkb::QueueType::present).value();
}

VulkanDevice::~VulkanDevice()
{
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkDestroyInstance(_instance, nullptr);
}

} // namespace vk
} // namespace baldwin
