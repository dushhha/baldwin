#pragma once

#include "renderer/renderer.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vulkan_swapchain.hpp"
#include "vulkan_device.hpp"

namespace baldwin
{
namespace vk
{

struct FrameData
{
    VkCommandPool commandPool = VK_NULL_HANDLE;
    VkCommandBuffer mainCommandBuffer = VK_NULL_HANDLE;
    VkFence renderFence = VK_NULL_HANDLE;
    VkSemaphore swapSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderSemaphore = VK_NULL_HANDLE;
    DeletionQueue deletionQueue = {};
};

class VulkanRenderer : public Renderer
{
  public:
    VulkanRenderer(GLFWwindow* window, int width, int height,
                   bool tripleBuffering = false);
    ~VulkanRenderer();
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    void render(int frameNum) override;

  private:
    void initCommands();
    void initSync();
    // void initPBRPipeline();
    void draw(int frameNum);

    VulkanDevice _device;
    VulkanSwapchain _swapchain;

    DeletionQueue _deletionQueue{};
    std::vector<FrameData> _frames{};
    int _frameOverlap = 2;
    FrameData& getCurrentFrame(int frameNum)
    {
        return _frames[frameNum % _frameOverlap];
    }
};

} // namespace vk
} // namespace baldwin
