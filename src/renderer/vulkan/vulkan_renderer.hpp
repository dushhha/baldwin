#pragma once

#include "renderer/renderer.hpp"

#include <deque>
#include <functional>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vulkan_swapchain.hpp"
#include "vulkan_device.hpp"
#include "vulkan_types.hpp"

namespace baldwin
{
namespace vk
{

struct DeletionQueue
{
    std::deque<std::function<void()>> deletors;

    void pushFunction(std::function<void()>&& function)
    {
        deletors.push_back(function);
    }

    void flush()
    {
        // reverse iterate the deletion queue to execute all the functions
        for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            (*it)();
        deletors.clear();
    }
};

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
    ~VulkanRenderer() override;
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;

    void render(int frameNum) override;

  private:
    void initCommands();
    void initSync();
    void initDefaultImages();
    // void initPBRPipeline();
    void draw(int frameNum);

    VulkanDevice _device;
    VulkanSwapchain _swapchain;
    Image _drawImage{};
    Image _depthImage{};
    VkExtent2D _drawExtent{ 0, 0 };

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
