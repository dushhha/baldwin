#include "vulkan_device.hpp"

#include <cmath>
#include <iostream>
#include <VkBootstrap.h>
#include <vulkan/vulkan_core.h>

#include "vulkan_infos.hpp"
#include "vulkan_utils.hpp"

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
    _debugMessenger = vkbInst.debug_messenger;

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

    // Allocator
    VmaAllocatorCreateInfo allocatorInfo = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = physicalDevice,
        .device = _device,
        .instance = _instance,
    };
    VK_CHECK(vmaCreateAllocator(&allocatorInfo, &_allocator),
             "Could not create VMA Allocator");

    initImmediate();
}

void VulkanDevice::initImmediate()
{
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _queueFamilies.graphics
    };
    VK_CHECK(
      vkCreateCommandPool(_device, &poolInfo, nullptr, &_immediateCommandPool),
      "Could not create immediate command pool");

    VkCommandBufferAllocateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _immediateCommandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    VK_CHECK(
      vkAllocateCommandBuffers(_device, &bufferInfo, &_immediateCommandBuffer),
      "Could not allocate immediate command buffer");

    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };
    VK_CHECK(vkCreateFence(_device, &fenceInfo, nullptr, &_immediateFence),
             "Could not create immediate fence");
}

void VulkanDevice::immediateSubmit(
  std::function<void(VkCommandBuffer cmd)>&& function)
{
    VK_CHECK(vkResetFences(_device, 1, &_immediateFence),
             "Could not reset immediate fence");
    VK_CHECK(vkResetCommandBuffer(_immediateCommandBuffer, 0),
             "Could not reset immediate command buffer");

    VkCommandBuffer cmd = _immediateCommandBuffer;

    VkCommandBufferBeginInfo beginInfo = getCommandBufferBeginInfo(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo),
             "Could not begin immediate command buffer");

    // Execute the user sent function
    function(cmd);

    VK_CHECK(vkEndCommandBuffer(cmd), "Could not end immediate command buffer");

    VkCommandBufferSubmitInfo cmdinfo = getCommandBufferSubmitInfo(cmd);
    VkSubmitInfo2 submit = getSubmitInfo(&cmdinfo, nullptr, nullptr);

    // Submit command buffer to the queue and execute it.
    // the fence will now block until the graphic commands finish execution
    VK_CHECK(vkQueueSubmit2(_graphicsQueue, 1, &submit, _immediateFence),
             "Could not submit immediate operations to queue");

    VK_CHECK(vkWaitForFences(_device, 1, &_immediateFence, true, 9999999999),
             "Could not wait for immediate fence");
}

Image VulkanDevice::createImage(VkExtent3D size, VkFormat format,
                                VkImageUsageFlags usage, bool mipmapped)
{
    Image newImage = {};
    newImage.format = format;
    newImage.extent = size;

    VkImageCreateInfo imgInfo = getImageCreateInfo(format, usage, size);
    if (mipmapped)
    {
        imgInfo.mipLevels = static_cast<uint32_t>(std::floor(
                              std::log2(std::max(size.width, size.height)))) +
                            1;
    }

    // always allocate images on dedicated GPU memory
    VmaAllocationCreateInfo allocinfo = {};
    allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocinfo.requiredFlags = static_cast<VkMemoryPropertyFlags>(
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    VK_CHECK(vmaCreateImage(_allocator,
                            &imgInfo,
                            &allocinfo,
                            &newImage.handle,
                            &newImage.allocation,
                            nullptr),
             "Could not create image");

    // if the format is a depth format, we will need to have it use the
    // correct aspect flag
    VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
    if (format == VK_FORMAT_D32_SFLOAT)
    {
        aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
    }

    // build a image-view for the image
    VkImageViewCreateInfo viewInfo = getImageViewCreateInfo(
      format, newImage.handle, aspectFlag);
    viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

    VK_CHECK(vkCreateImageView(_device, &viewInfo, nullptr, &newImage.view),
             "Could not create image view");

    return newImage;
}

void VulkanDevice::destroyImage(Image& image)
{
    vkDestroyImageView(_device, image.view, nullptr);
    vmaDestroyImage(_allocator, image.handle, image.allocation);
}

Buffer VulkanDevice::createBuffer(size_t size, VkBufferUsageFlags usageFlags,
                                  VmaMemoryUsage memoryUsage)
{
    VkBufferCreateInfo bufferInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usageFlags,
    };
    VmaAllocationCreateInfo allocInfo = {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = memoryUsage,
    };

    Buffer newBuffer = {};
    VK_CHECK(vmaCreateBuffer(_allocator,
                             &bufferInfo,
                             &allocInfo,
                             &newBuffer.handle,
                             &newBuffer.allocation,
                             &newBuffer.allocationInfo),
             "Could not create buffer");

    return newBuffer;
}

void VulkanDevice::destroyBuffer(Buffer& buffer)
{
    vmaDestroyBuffer(_allocator, buffer.handle, buffer.allocation);
}

VkShaderModule VulkanDevice::createShaderModule(const std::vector<char>& code)
{
    VkShaderModule module;
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = code.size(),
        .pCode = reinterpret_cast<const uint32_t*>(code.data()),
    };
    VK_CHECK(vkCreateShaderModule(_device, &createInfo, nullptr, &module),
             "Could not create shader module");

    return module;
}

void VulkanDevice::destroyShaderModule(VkShaderModule& module)
{
    vkDestroyShaderModule(_device, module, nullptr);
}

VulkanDevice::~VulkanDevice()
{
#ifndef NDEBUG
    std::cout << "-- VulkanDevice cleanup\n";
#endif

    vkDestroyFence(_device, _immediateFence, nullptr);
    vkFreeCommandBuffers(
      _device, _immediateCommandPool, 1, &_immediateCommandBuffer);
    vkDestroyCommandPool(_device, _immediateCommandPool, nullptr);
    vmaDestroyAllocator(_allocator);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkb::destroy_debug_utils_messenger(_instance, _debugMessenger);
    vkDestroyInstance(_instance, nullptr);
}

} // namespace vk
} // namespace baldwin
