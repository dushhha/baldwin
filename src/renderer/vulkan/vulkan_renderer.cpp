#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#define GLFW_INCLUDE_VULKAN
#define VMA_IMPLEMENTATION

#include "vulkan_renderer.hpp"

#include <iostream>
#include <cmath>
#include <cassert>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include <glm/gtx/transform.hpp>

#include "vulkan_images.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_infos.hpp"
#include "vulkan_pipelines.hpp"
#include "vulkan_descriptors.hpp"
#include "renderer/shaders.hpp"
#include "renderer/render_types.hpp"

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
    initRenderTargets();
    initDefaultData();
    initSceneDescriptors();
    initDiffusePipeline();
}

void VulkanRenderer::initCommands()
{
    assert(_device.handle() != VK_NULL_HANDLE);

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
          [&, i]()
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
    assert(_device.handle() != VK_NULL_HANDLE);

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
          [&, i]()
          {
              vkDestroyFence(_device.handle(), _frames[i].renderFence, nullptr);
              vkDestroySemaphore(
                _device.handle(), _frames[i].renderSemaphore, nullptr);
              vkDestroySemaphore(
                _device.handle(), _frames[i].swapSemaphore, nullptr);
          });
    }
}

void VulkanRenderer::initRenderTargets()
{
    assert(_device.handle() != VK_NULL_HANDLE);
    assert(_swapchain.handle() != VK_NULL_HANDLE);

    VkExtent3D size = { 1600, 900, 1 };
    VkImageUsageFlags colorUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                   VK_IMAGE_USAGE_STORAGE_BIT |
                                   VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                   VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    _drawImage = _device.createImage(
      size, VK_FORMAT_R16G16B16A16_SFLOAT, colorUsage, false);

    VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    _depthImage = _device.createImage(
      size, VK_FORMAT_D32_SFLOAT, depthUsage, false);

    _deletionQueue.pushFunction(
      [&]()
      {
          _device.destroyImage(_drawImage);
          _device.destroyImage(_depthImage);
      });
}

void VulkanRenderer::initDefaultData()
{
    // Scene uniform buffer
    assert(_device.handle() != VK_NULL_HANDLE);
    _sceneUniformBuffer = _device.createBuffer(
      sizeof(SceneData),
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

    _deletionQueue.pushFunction(
      [&]()
      {
          _device.destroyBuffer(_sceneUniformBuffer);
      });
}

void VulkanRenderer::initSceneDescriptors()
{
    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

    VkDescriptorBindingFlags
      bindingFlag = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT;
    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo = {
        .sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &bindingFlag
    };
    _sceneLayout = layoutBuilder.build(
      _device.handle(),
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
      &bindingFlagsInfo,
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT);

    std::vector<DescriptorManager::PoolSizeRatio> ratios = {
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 }
    };
    _descriptorManager.init(_device.handle(), 10, ratios);
    _sceneSet = _descriptorManager.allocate(
      _device.handle(), _sceneLayout, nullptr);

    _deletionQueue.pushFunction(
      [&]()
      {
          _descriptorManager.destroyPools(_device.handle());
          vkDestroyDescriptorSetLayout(_device.handle(), _sceneLayout, nullptr);
      });
}

void VulkanRenderer::initDiffusePipeline()
{
    assert(_device.handle() != VK_NULL_HANDLE);
    assert(_sceneLayout != VK_NULL_HANDLE);

    // Pipeline layout
    VkPushConstantRange range = { .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                                  .offset = 0,
                                  .size = sizeof(RasterizePushConstants) };

    VkPipelineLayoutCreateInfo diffuseLayout = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &_sceneLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range
    };
    VK_CHECK(
      vkCreatePipelineLayout(
        _device.handle(), &diffuseLayout, nullptr, &_diffusePipelineLayout),
      "Could not create diffuse pipeline layout");

    // Modules
    auto vertCode = readShaderFile("shaders/diffuse.vert.spv");
    assert(!vertCode.empty());
    VkShaderModule vertModule = _device.createShaderModule(vertCode);

    auto fragCode = readShaderFile("shaders/diffuse.frag.spv");
    assert(!fragCode.empty());
    VkShaderModule fragModule = _device.createShaderModule(fragCode);

    // Pipeline
    GraphicsPipelineBuilder builder = {};
    builder._pipelineLayout = _diffusePipelineLayout;
    builder.setShaders(vertModule, fragModule);
    builder.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    builder.setPolygonMode(VK_POLYGON_MODE_FILL);
    builder.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    builder.disableMultiSampling();
    builder.disableBlending();
    builder.setColorAttachment(_drawImage.format);
    builder.setDepthFormat(_depthImage.format);
    builder.enableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
    _diffusePipeline = builder.build(_device.handle());

    _device.destroyShaderModule(fragModule);
    _device.destroyShaderModule(vertModule);

    _deletionQueue.pushFunction(
      [&]()
      {
          vkDestroyPipelineLayout(
            _device.handle(), _diffusePipelineLayout, nullptr);
          vkDestroyPipeline(_device.handle(), _diffusePipeline, nullptr);
      });
}

void VulkanRenderer::resizeSwapchain(int width, int height)
{
    _swapchain.reconstruct(width, height);
}

void VulkanRenderer::uploadMesh(const std::shared_ptr<Mesh> mesh)
{
    if (!_meshBuffersMap.contains(mesh->uuid))
    {
        size_t vertexSize = mesh->vertices.size() * sizeof(Vertex);
        size_t indexSize = mesh->indices.size() * sizeof(uint32_t);

        // Temporary buffer
        Buffer staging = _device.createBuffer(vertexSize + indexSize,
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VMA_MEMORY_USAGE_CPU_ONLY);
        void* data = staging.allocation->GetMappedData();

        // Copy date on the cpu staging buffer
        memcpy((char*)data, mesh->vertices.data(), vertexSize);
        memcpy((char*)data + vertexSize, mesh->indices.data(), indexSize);

        MeshBuffers buffers;
        buffers.vertexBuffer = _device.createBuffer(
          vertexSize,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
          VMA_MEMORY_USAGE_CPU_ONLY);
        buffers.indexBuffer = _device.createBuffer(
          indexSize,
          VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
          VMA_MEMORY_USAGE_CPU_ONLY);

        VkBufferDeviceAddressInfo deviceAdressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffers.vertexBuffer.handle
        };
        buffers.vertexBufferAddress = vkGetBufferDeviceAddress(
          _device.handle(), &deviceAdressInfo);

        _device.immediateSubmit(
          [&](VkCommandBuffer cmd)
          {
              VkBufferCopy vertexCopy = {
                  .srcOffset = 0,
                  .dstOffset = 0,
                  .size = vertexSize,
              };
              vkCmdCopyBuffer(cmd,
                              staging.handle,
                              buffers.vertexBuffer.handle,
                              1,
                              &vertexCopy);

              VkBufferCopy indexCopy = {
                  .srcOffset = vertexSize,
                  .dstOffset = 0,
                  .size = indexSize,
              };
              vkCmdCopyBuffer(
                cmd, staging.handle, buffers.indexBuffer.handle, 1, &indexCopy);
          });
        _device.destroyBuffer(staging);
        _meshBuffersMap[mesh->uuid] = buffers;
    }
}

void VulkanRenderer::updateSceneBuffer(const VkCommandBuffer& cmd)
{
    // TODO: Replace dummy data by real scene data
    SceneData dummySceneData = {};
    dummySceneData.view = glm::translate(glm::vec3{ 0, 0, -3 });
    dummySceneData.proj = glm::perspective(glm::radians(70.f),
                                           (float)_drawExtent.width /
                                             (float)_drawExtent.height,
                                           0.1f,
                                           10.0f);

    dummySceneData.proj[1][1] *= -1;
    dummySceneData.viewproj = dummySceneData.proj * dummySceneData.view;

    dummySceneData.ambientColor = glm::vec4(0.8, 0.8, 0.9, 0.3);
    dummySceneData.sunlightDirection = glm::vec4(0.5, 1.0, 0.2, 0.0);
    dummySceneData.sunlightColor = glm::vec4(1.0, 1.0, 0.9, 1.0);

    SceneData* bufferSceneData = static_cast<SceneData*>(
      _sceneUniformBuffer.allocation->GetMappedData());
    *bufferSceneData = dummySceneData;

    DescriptorWriter writer{};
    writer.writeBuffer(0,
                       _sceneUniformBuffer.handle,
                       _sceneUniformBuffer.allocationInfo.size,
                       0,
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.updateSet(_device.handle(), _sceneSet);
}

void VulkanRenderer::drawObjects(
  const VkCommandBuffer& cmd, const std::vector<std::shared_ptr<Mesh>>& scene)
{
    // Begin a render pass connected to our draw image
    VkRenderingAttachmentInfo colorAttachment = getAttachmentInfo(
      _drawImage.view, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo depthAttachment = getDepthAttachmentInfo(
      _depthImage.view, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    VkRenderingInfo renderInfo = getRenderingInfo(
      _drawExtent, &colorAttachment, &depthAttachment);
    vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _diffusePipeline);
    vkCmdBindDescriptorSets(cmd,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            _diffusePipelineLayout,
                            0,
                            1,
                            &_sceneSet,
                            0,
                            nullptr);

    // Dynamic viewport and scissor
    // The vp defines the transformation from the image to the framebuffer
    // The scissor rectangle define in which which regions pixels will
    // actually be stored vp -> fit, scissor -> crop
    VkViewport vp = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(_drawExtent.width),
        .height = static_cast<float>(_drawExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(cmd, 0, 1, &vp);

    VkRect2D scissor = { .offset = { 0, 0 }, .extent = _drawExtent };
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    for (auto& mesh : scene)
    {
        if (!_meshBuffersMap.contains(mesh->uuid))
            continue;

        auto meshBuffers = _meshBuffersMap[mesh->uuid];
        RasterizePushConstants pc = {
            .vertexBufferAddress = meshBuffers.vertexBufferAddress
        };
        pc.worldMatrix = glm::identity<glm::mat4>();
        vkCmdPushConstants(cmd,
                           _diffusePipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(RasterizePushConstants),
                           &pc);

        vkCmdBindIndexBuffer(
          cmd, meshBuffers.indexBuffer.handle, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmd, mesh->indices.size(), 1, 0, 0, 0);
    }
    vkCmdEndRendering(cmd);
}

void VulkanRenderer::draw(int frameNum,
                          const std::vector<std::shared_ptr<Mesh>>& scene)
{
    // Wait for GPU to finish rendering
    vkWaitForFences(_device.handle(),
                    1,
                    &getCurrentFrame(frameNum).renderFence,
                    VK_TRUE,
                    1000000000);

    // Request swapchain image index that we can blit on
    uint32_t swapchainImgIndex;
    if (!_swapchain.sane)
        return;

    VkResult r = vkAcquireNextImageKHR(_device.handle(),
                                       _swapchain.handle(),
                                       1000000,
                                       getCurrentFrame(frameNum).swapSemaphore,
                                       nullptr,
                                       &swapchainImgIndex);

    if (r == VK_ERROR_OUT_OF_DATE_KHR)
        _swapchain.sane = false;
    else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR)
        throw(std::runtime_error("Could not acquire swapchain image"));

    _drawExtent.height = std::min(_swapchain.extent().height,
                                  _drawImage.extent.height);
    _drawExtent.width = std::min(_swapchain.extent().width,
                                 _drawImage.extent.width);

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
                                     _drawImage.handle,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_GENERAL);

    VkClearColorValue clearValue = { 0.1, 0.1, 0.1, 1.0 };
    VkImageSubresourceRange srcRange = getImageSubresourceRange(
      VK_IMAGE_ASPECT_COLOR_BIT);
    vkCmdClearColorImage(cmd,
                         _drawImage.handle,
                         VK_IMAGE_LAYOUT_GENERAL,
                         &clearValue,
                         1,
                         &srcRange);

    createImageBarrierWithTransition(cmd,
                                     _drawImage.handle,
                                     VK_IMAGE_LAYOUT_GENERAL,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    createImageBarrierWithTransition(cmd,
                                     _depthImage.handle,
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    updateSceneBuffer(cmd);
    drawObjects(cmd, scene);

    createImageBarrierWithTransition(cmd,
                                     _drawImage.handle,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                     VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    createImageBarrierWithTransition(cmd,
                                     _swapchain.image(swapchainImgIndex),
                                     VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copyImageToImage(cmd,
                     _drawImage.handle,
                     _swapchain.image(swapchainImgIndex),
                     _drawExtent,
                     _swapchain.extent());

    createImageBarrierWithTransition(cmd,
                                     _swapchain.image(swapchainImgIndex),
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
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
    if (r == VK_ERROR_OUT_OF_DATE_KHR)
        _swapchain.sane = false;
    else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR)
        throw(std::runtime_error("Could not acquire swapchain image"));
}

void VulkanRenderer::render(int frameNum,
                            const std::vector<std::shared_ptr<Mesh>>& scene)
{
    draw(frameNum, scene);
}

VulkanRenderer::~VulkanRenderer()
{
#ifndef NDEBUG
    std::cout << "- VulkanRenderer cleanup\n";
#endif
    vkDeviceWaitIdle(_device.handle());
    for (auto& frame : _frames)
    {
        frame.deletionQueue.flush();
    }
    while (!_meshBuffersMap.empty())
    {
        auto it = _meshBuffersMap
                    .begin(); // Always pick the first available element
        _device.destroyBuffer(it->second.vertexBuffer);
        _device.destroyBuffer(it->second.indexBuffer);
        _meshBuffersMap.erase(it);
    }
    _deletionQueue.flush();
}

} // namespace vk
} // namespace baldwin
