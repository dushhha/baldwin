#pragma once

#include <vector>
#include <vulkan/vulkan.h>

namespace baldwin
{
namespace vk
{
class GraphicsPipelineBuilder
{
  public:
    GraphicsPipelineBuilder() { clear(); }
    void clear();
    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
    void setInputTopology(VkPrimitiveTopology topology);
    void setPolygonMode(VkPolygonMode mode);
    void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
    void disableMultiSampling();
    void disableBlending();
    void enableBlendingAdditive();
    void enableBlendingAlphablend();
    void setColorAttachment(VkFormat format);
    void setDepthFormat(VkFormat format);
    void disableDepthTest();
    void enableDepthTest(bool depthWriteEnable, VkCompareOp op);
    VkPipeline build(const VkDevice& device);

    VkPipelineLayout _pipelineLayout;

  private:
    std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
    VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
    VkPipelineRasterizationStateCreateInfo _rasterizer;
    VkPipelineColorBlendAttachmentState _colorBlendAttachment;
    VkPipelineMultisampleStateCreateInfo _multisampling;
    VkPipelineDepthStencilStateCreateInfo _depthStencil;
    VkPipelineRenderingCreateInfo _renderInfo;
    VkFormat _colorAttachmentformat;
};

} // namespace vk
} // namespace baldwin
