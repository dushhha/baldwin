#include "vulkan_infos.hpp"

namespace baldwin
{
namespace vk
{

VkSemaphoreSubmitInfo getSemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask,
                                             VkSemaphore semaphore)
{
    VkSemaphoreSubmitInfo submitInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = semaphore,
        .value = 1,
        .stageMask = stageMask,
        .deviceIndex = 0,
    };

    return submitInfo;
}

VkCommandBufferBeginInfo getCommandBufferBeginInfo(
  VkCommandBufferUsageFlags flags)
{
    VkCommandBufferBeginInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = flags,
        .pInheritanceInfo = nullptr
    };
    return info;
}

VkCommandBufferSubmitInfo getCommandBufferSubmitInfo(VkCommandBuffer cmd)
{
    VkCommandBufferSubmitInfo info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmd,
        .deviceMask = 0,
    };
    return info;
}

VkSubmitInfo2 getSubmitInfo(VkCommandBufferSubmitInfo* cmd,
                            VkSemaphoreSubmitInfo* signalSemaphoreInfo,
                            VkSemaphoreSubmitInfo* waitSemaphoreInfo)
{
    VkSubmitInfo2 info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,

        .waitSemaphoreInfoCount = static_cast<uint32_t>(
          waitSemaphoreInfo == nullptr ? 0 : 1),
        .pWaitSemaphoreInfos = waitSemaphoreInfo,

        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = cmd,

        .signalSemaphoreInfoCount = static_cast<uint32_t>(
          signalSemaphoreInfo == nullptr ? 0 : 1),
        .pSignalSemaphoreInfos = signalSemaphoreInfo,
    };
    return info;
}

VkRenderingAttachmentInfo getAttachmentInfo(VkImageView view,
                                            VkClearValue* clear,
                                            VkImageLayout layout)
{
    VkRenderingAttachmentInfo colorAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = view,
        .imageLayout = layout,
        .loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR
                        : VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    if (clear)
    {
        colorAttachment.clearValue = *clear;
    }

    return colorAttachment;
}

VkRenderingAttachmentInfo getDepthAttachmentInfo(VkImageView view,
                                                 VkImageLayout layout)
{
    VkRenderingAttachmentInfo depthAttachment = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = view,
        .imageLayout = layout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
    };
    depthAttachment.clearValue.depthStencil.depth = 1.0f;

    return depthAttachment;
}

VkRenderingInfo getRenderingInfo(VkExtent2D renderExtent,
                                 VkRenderingAttachmentInfo* colorAttachment,
                                 VkRenderingAttachmentInfo* depthAttachment)
{
    VkRenderingInfo info = { .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
                             .renderArea = VkRect2D{ VkOffset2D{ 0, 0 },
                                                     renderExtent },
                             .layerCount = 1,
                             .colorAttachmentCount = 1,
                             .pColorAttachments = colorAttachment,
                             .pDepthAttachment = depthAttachment,
                             .pStencilAttachment = nullptr };

    return info;
}

VkImageCreateInfo getImageCreateInfo(VkFormat format,
                                     VkImageUsageFlags usageFlags,
                                     VkExtent3D extent, bool cubeMap)
{
    VkImageCreateInfo imgCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = cubeMap ? 6u : 1u,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usageFlags
    };
    if (cubeMap)
    {
        imgCreateInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    return imgCreateInfo;
}

VkImageViewCreateInfo getImageViewCreateInfo(VkFormat format, VkImage image,
                                             VkImageAspectFlags aspectFlags,
                                             bool cubeMap)
{
    VkImageViewCreateInfo imgViewCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = cubeMap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
    };
    imgViewCreateInfo.subresourceRange.aspectMask = aspectFlags;
    imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    imgViewCreateInfo.subresourceRange.levelCount = 1;
    imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    imgViewCreateInfo.subresourceRange.layerCount = cubeMap ? 6u : 1u;

    return imgViewCreateInfo;
}

VkPipelineShaderStageCreateInfo getPipelineShaderStageCreateInfo(
  VkShaderStageFlagBits stage, VkShaderModule module, const char* name)
{
    VkPipelineShaderStageCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = name
    };
    return info;
}

} // namespace vk
} // namespace baldwin
