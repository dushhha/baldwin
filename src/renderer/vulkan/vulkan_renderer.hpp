#pragma once

#include "renderer/renderer.hpp"

#include <deque>
#include <string>
#include <functional>
#include <GLFW/glfw3.h>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <glm/mat4x4.hpp>

#include "vulkan_swapchain.hpp"
#include "vulkan_device.hpp"
#include "vulkan_types.hpp"
#include "scene/mesh.hpp"

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

    void resizeSwapchain(int width, int height) override;
    void uploadMesh(const std::shared_ptr<Mesh> mesh) override;
    void render(int frameNum,
                const std::vector<std::shared_ptr<Mesh>>& scene) override;

  private:
    void initCommands();
    void initSync();
    void initDefaultImages();
    void initVertexColorPipeline();
    void drawObjects(const VkCommandBuffer& cmd,
                     const std::vector<std::shared_ptr<Mesh>>& scene);
    void draw(int frameNum, const std::vector<std::shared_ptr<Mesh>>& scene);

    VulkanDevice _device;
    VulkanSwapchain _swapchain;
    DeletionQueue _deletionQueue{};
    Image _drawImage{};
    Image _depthImage{};
    VkExtent2D _drawExtent{ 0, 0 };

    struct TempPushConstant
    {
        glm::mat4x4 worldMatrix;
        VkDeviceAddress vertexBufferAddress;
    };

    VkPipeline _vertexColorPipeline = VK_NULL_HANDLE;
    VkPipelineLayout _vertexColorPipelineLayout = VK_NULL_HANDLE;

    std::unordered_map<std::string, MeshBuffers> _meshBuffersMap;

    int _frameOverlap = 2;
    std::vector<FrameData> _frames{};
    FrameData& getCurrentFrame(int frameNum)
    {
        return _frames[frameNum % _frameOverlap];
    }
};

} // namespace vk
} // namespace baldwin
