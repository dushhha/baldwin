#pragma ocne

#include "renderer/renderer.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

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

    void run(int frame) override;

  private:
    DeletionQueue _deletionQueue{};
    std::vector<FrameData> _frames{};
    // TODO: init this
    const int _frameOverlap = 2;
    FrameData& getCurrentFrame(int frameNum)
    {
        return _frames[frameNum % _frameOverlap];
    }
};

} // namespace vk
} // namespace baldwin
