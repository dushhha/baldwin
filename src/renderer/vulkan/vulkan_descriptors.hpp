#pragma once

#include <deque>
#include <vector>
#include <span>
#include <vulkan/vulkan.h>

namespace baldwin
{
namespace vk
{

struct DescriptorLayoutBuilder
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    void addBinding(uint32_t binding, VkDescriptorType type,
                    uint32_t descriptorCount = 1);
    void clear();
    VkDescriptorSetLayout build(VkDevice device,
                                VkShaderStageFlags shaderStages,
                                void* pNext = nullptr,
                                VkDescriptorSetLayoutCreateFlags flags = 0);
};

struct DescriptorWriter
{
    std::deque<VkDescriptorImageInfo> imageInfos;
    std::deque<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkWriteDescriptorSet> writes;

    void writeImage(int binding, VkImageView image, VkSampler sampler,
                    VkImageLayout layout, VkDescriptorType type);
    void writeBuffer(int binding, VkBuffer buffer, size_t size, size_t offset,
                     VkDescriptorType type);

    void clear();
    void updateSet(VkDevice device, VkDescriptorSet set);
};

struct DescriptorManager
{
  public:
    struct PoolSizeRatio
    {
        VkDescriptorType type;
        float ratio;
    };

    void init(VkDevice device, uint32_t initialSets,
              std::span<PoolSizeRatio> poolRatios);
    void clearPools(VkDevice device);
    void destroyPools(VkDevice device);

    VkDescriptorSet allocate(VkDevice device, VkDescriptorSetLayout layout,
                             void* pNext = nullptr);

  private:
    VkDescriptorPool getPool(VkDevice device);
    VkDescriptorPool createPool(VkDevice device, uint32_t setCount,
                                std::span<PoolSizeRatio> poolRatios);

    std::vector<PoolSizeRatio> ratios;
    std::vector<VkDescriptorPool> fullPools;
    std::vector<VkDescriptorPool> readyPools;
    uint32_t setsPerPool;
};

} // namespace vk
} // namespace baldwin
