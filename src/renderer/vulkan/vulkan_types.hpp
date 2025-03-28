#pragma once

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/mat4x4.hpp>

namespace baldwin
{
namespace vk
{

struct Buffer
{
    VkBuffer handle = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo allocationInfo = {};
};

struct Image
{
    VkImage handle = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VkExtent3D extent = {};
    VkFormat format = VK_FORMAT_UNDEFINED;
};

struct MeshBuffers
{
    Buffer vertexBuffer;
    Buffer indexBuffer;
    VkDeviceAddress vertexBufferAddress;
};

struct RasterizePushConstants
{
    glm::mat4x4 worldMatrix;
    VkDeviceAddress vertexBufferAddress;
};

} // namespace vk
} // namespace baldwin
