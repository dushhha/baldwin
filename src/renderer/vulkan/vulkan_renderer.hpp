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

#include "renderer/vulkan/vulkan_descriptors.hpp"
#include "vulkan_swapchain.hpp"
#include "vulkan_device.hpp"
#include "vulkan_types.hpp"
#include "renderer/render_types.hpp"

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
    void initRenderTargets();
    void initDefaultData();
    void initSceneDescriptors();
    void initDiffusePipeline();
    void updateSceneBuffer(const VkCommandBuffer& cmd);
    void drawObjects(const VkCommandBuffer& cmd,
                     const std::vector<std::shared_ptr<Mesh>>& scene);
    void draw(int frameNum, const std::vector<std::shared_ptr<Mesh>>& scene);

    VulkanDevice _device;
    VulkanSwapchain _swapchain;
    DeletionQueue _deletionQueue{};
    Image _drawImage{};
    Image _depthImage{};
    VkExtent2D _drawExtent{ 0, 0 };

    DescriptorManager _descriptorManager{};
    VkDescriptorSetLayout _sceneLayout = VK_NULL_HANDLE;
    VkDescriptorSet _sceneSet = VK_NULL_HANDLE;
    VkPipeline _diffusePipeline = VK_NULL_HANDLE;
    VkPipelineLayout _diffusePipelineLayout = VK_NULL_HANDLE;

    Buffer _sceneUniformBuffer{};
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
