#define VMA_IMPLEMENTATION
#define GLFW_INCLUDE_VULKAN
#include "vulkan_renderer.hpp"

#include <iostream>
#include <cmath>

#include "vulkan_images.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_infos.hpp"

namespace baldwin
{
namespace vk
{

VulkanRenderer::VulkanRenderer(GLFWwindow* window, int width, int height,
                               bool tripleBuffering)
  : _device{ window }
  , _swapchain(_device, width, height)
{
    if (tripleBuffering)
        _frameOverlap = 3;

    initCommands();
    initSync();
}

void VulkanRenderer::initCommands()
{
    // Frames
    VkCommandPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = _device.queueFamilies().graphics
    };

    for (int i = 0; i < _frameOverlap; i++)
    {
        FrameData frame{};
        VK_CHECK(vkCreateCommandPool(
                   _device.handle(), &poolInfo, nullptr, &frame.commandPool),
                 "Could not create frame command pool");

        VkCommandBufferAllocateInfo bufferInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = frame.commandPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VK_CHECK(vkAllocateCommandBuffers(
                   _device.handle(), &bufferInfo, &frame.mainCommandBuffer),
                 "Could not allocate frame main command buffer");

        _frames.push_back(frame);
        _frames[i].deletionQueue.pushFunction(
          [this, i]()
          {
              vkFreeCommandBuffers(_device.handle(),
                                   _frames[i].commandPool,
                                   1,
                                   &_frames[i].mainCommandBuffer);
              vkDestroyCommandPool(
                _device.handle(), _frames[i].commandPool, nullptr);
          });
    }
}

void VulkanRenderer::initSync()
{
    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for (int i = 0; i < _frameOverlap; i++)
    {
        VK_CHECK(vkCreateSemaphore(_device.handle(),
                                   &semaphoreInfo,
                                   nullptr,
                                   &_frames[i].swapSemaphore),
                 "Could not create frame swapSemaphore");
        VK_CHECK(vkCreateSemaphore(_device.handle(),
                                   &semaphoreInfo,
                                   nullptr,
                                   &_frames[i].renderSemaphore),
                 "Could not create frame renderSemaphore");
        VK_CHECK(
          vkCreateFence(
            _device.handle(), &fenceInfo, nullptr, &_frames[i].renderFence),
          "Could not create frame renderFence");

        _frames[i].deletionQueue.pushFunction(
          [this, i]()
          {
              vkDestroyFence(_device.handle(), _frames[i].renderFence, nullptr);
              vkDestroySemaphore(
                _device.handle(), _frames[i].renderSemaphore, nullptr);
              vkDestroySemaphore(
                _device.handle(), _frames[i].swapSemaphore, nullptr);
          });
    }
}

void VulkanRenderer::draw(int frameNum)
{
    // Wait for GPU to finish rendering
    vkWaitForFences(_device.handle(),
                    1,
                    &getCurrentFrame(frameNum).renderFence,
                    VK_TRUE,
                    1000000000);

    // Request swapchain image index that we can blit on
    uint32_t swapchainImgIndex;
    VkResult r = vkAcquireNextImageKHR(_device.handle(),
                                       _swapchain.handle(),
                                       1000000,
                                       getCurrentFrame(frameNum).swapSemaphore,
                                       nullptr,
                                       &swapchainImgIndex);

    vkResetFences(_device.handle(), 1, &getCurrentFrame(frameNum).renderFence);

    // Usual command workflow is : 1. wait / 2. reset / 3. begin / 4. record
    // / 5. submit to queue
    VkCommandBuffer cmd = getCurrentFrame(frameNum).mainCommandBuffer;
    VK_CHECK(vkResetCommandBuffer(cmd, 0), "Could not reset command buffer");
    VkCommandBufferBeginInfo cmdBeginInfo = getCommandBufferBeginInfo(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo),
             "Could not begin command recording");

    createImageBarrierWithTransition(cmd,
                                     _swapchain.image(swapchainImgIndex),
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_GENERAL);

    float bValue = 0.5 + 0.5 * std::sin(static_cast<float>(frameNum) / 120.0);
    VkClearColorValue clearValue = { 0.0, 0.0, bValue, 1.0 };
    VkImageSubresourceRange srcRange = getImageSubresourceRange(
      VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd,
                         _swapchain.image(swapchainImgIndex),
                         VK_IMAGE_LAYOUT_GENERAL,
                         &clearValue,
                         1,
                         &srcRange);

    createImageBarrierWithTransition(cmd,
                                     _swapchain.image(swapchainImgIndex),
                                     VK_IMAGE_LAYOUT_GENERAL,
                                     VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    VK_CHECK(vkEndCommandBuffer(cmd), "Could not end command recording");

    // We finished drawing, time to submit
    VkCommandBufferSubmitInfo cmdSubmitInfo = getCommandBufferSubmitInfo(cmd);
    VkSemaphoreSubmitInfo waitInfo = getSemaphoreSubmitInfo(
      VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
      getCurrentFrame(frameNum).swapSemaphore);
    VkSemaphoreSubmitInfo signalInfo = getSemaphoreSubmitInfo(
      VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR,
      getCurrentFrame(frameNum).renderSemaphore);
    VkSubmitInfo2 submitInfo = getSubmitInfo(
      &cmdSubmitInfo, &signalInfo, &waitInfo);

    VK_CHECK(vkQueueSubmit2(_device.graphicsQueue(),
                            1,
                            &submitInfo,
                            getCurrentFrame(frameNum).renderFence),
             "Could not submit graphics commands to queue");

    // We wait for rendering operations to finish and we
    // present
    VkSwapchainKHR swapchainHandle = _swapchain.handle();
    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = nullptr,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &getCurrentFrame(frameNum).renderSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchainHandle,
        .pImageIndices = &swapchainImgIndex
    };
    r = vkQueuePresentKHR(_device.presentQueue(), &presentInfo);
}

void VulkanRenderer::render(int frameNum) { draw(frameNum); }

VulkanRenderer::~VulkanRenderer()
{
    vkDeviceWaitIdle(_device.handle());
    for (auto& frame : _frames)
    {
        frame.deletionQueue.flush();
    }
    _deletionQueue.flush();
}

} // namespace vk
} // namespace baldwin
