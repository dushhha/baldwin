#include "vulkan_descriptors.hpp"

#include "vulkan_utils.hpp"
#include <vulkan/vulkan_core.h>

namespace baldwin
{
namespace vk
{

// === Descriptor Layout Builder ===
void DescriptorLayoutBuilder::addBinding(uint32_t binding,
                                         VkDescriptorType type,
                                         uint32_t descriptorCount)
{
    VkDescriptorSetLayoutBinding newbind = {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = descriptorCount,
    };
    bindings.push_back(newbind);
}

void DescriptorLayoutBuilder::clear() { bindings.clear(); }

VkDescriptorSetLayout DescriptorLayoutBuilder::build(
  VkDevice device, VkShaderStageFlags shaderStages, void* pNext,
  VkDescriptorSetLayoutCreateFlags flags)
{
    for (auto& b : bindings)
    {
        b.stageFlags |= shaderStages;
    }

    VkDescriptorSetLayoutCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = pNext,
        .flags = flags,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
    };

    VkDescriptorSetLayout setLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &info, nullptr, &setLayout),
             "Could not create descriptor set layout");

    return setLayout;
}

// === Descriptor Wrtier ===
void DescriptorWriter::writeBuffer(int binding, VkBuffer buffer, size_t size,
                                   size_t offset, VkDescriptorType type)
{
    VkDescriptorBufferInfo& info = bufferInfos.emplace_back(
      VkDescriptorBufferInfo{
        .buffer = buffer, .offset = offset, .range = size });

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE, // left empty for now until we need to
                                  // write it
        .descriptorCount = 1,
        .descriptorType = type,
        .pBufferInfo = &info,
    };
    write.dstBinding = binding;

    writes.push_back(write);
}

void DescriptorWriter::writeImage(int binding, VkImageView imageView,
                                  VkSampler sampler, VkImageLayout layout,
                                  VkDescriptorType type)
{
    VkDescriptorImageInfo& info = imageInfos.emplace_back(VkDescriptorImageInfo{
      .sampler = sampler, .imageView = imageView, .imageLayout = layout });

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE, // left empty for now until we need to
                                  // write it
        .descriptorCount = 1,
        .descriptorType = type,
        .pImageInfo = &info,
    };
    write.dstBinding = binding;

    writes.push_back(write);
}

void DescriptorWriter::clear()
{
    imageInfos.clear();
    bufferInfos.clear();
    writes.clear();
}

void DescriptorWriter::updateSet(VkDevice device, VkDescriptorSet set)
{
    for (VkWriteDescriptorSet& write : writes)
    {
        write.dstSet = set;
    }
    vkUpdateDescriptorSets(
      device, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
}

// === Growable Descriptor Allocator ===
void DescriptorManager::init(VkDevice device, uint32_t maxSets,
                             std::span<PoolSizeRatio> poolRatios)
{
    ratios.clear();

    for (auto r : poolRatios)
    {
        ratios.push_back(r);
    }

    VkDescriptorPool newPool = createPool(device, maxSets, poolRatios);
    setsPerPool = maxSets * 1.5;
    readyPools.push_back(newPool);
}

void DescriptorManager::clearPools(VkDevice device)
{
    for (auto p : readyPools)
    {
        vkResetDescriptorPool(device, p, 0);
    }
    for (auto p : fullPools)
    {
        vkResetDescriptorPool(device, p, 0);
        readyPools.push_back(p);
    }
    fullPools.clear();
}

void DescriptorManager::destroyPools(VkDevice device)
{
    for (auto p : readyPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    readyPools.clear();
    for (auto p : fullPools)
    {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    fullPools.clear();
}

VkDescriptorSet DescriptorManager::allocate(VkDevice device,
                                            VkDescriptorSetLayout layout,
                                            void* pNext)
{
    // Get or create a pool to allocate from
    VkDescriptorPool poolToUse = getPool(device);

    VkDescriptorSetAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = pNext,
        .descriptorPool = poolToUse,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,

    };

    VkDescriptorSet ds;
    VkResult result = vkAllocateDescriptorSets(device, &allocInfo, &ds);

    // Allocation failed. Try again
    if (result == VK_ERROR_OUT_OF_POOL_MEMORY ||
        result == VK_ERROR_FRAGMENTED_POOL)
    {

        fullPools.push_back(poolToUse);

        poolToUse = getPool(device);
        allocInfo.descriptorPool = poolToUse;

        VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &ds),
                 "Could not allocate descriptor pool");
    }

    readyPools.push_back(poolToUse);
    return ds;
}

VkDescriptorPool DescriptorManager::getPool(VkDevice device)
{
    VkDescriptorPool newPool;
    if (readyPools.size() != 0)
    {
        newPool = readyPools.back();
        readyPools.pop_back();
    }
    else
    {
        // Need to create a new pool
        newPool = createPool(device, setsPerPool, ratios);

        setsPerPool = static_cast<uint32_t>(
          std::min(setsPerPool * 1.5, 4092.0));
    }
    return newPool;
}

VkDescriptorPool DescriptorManager::createPool(
  VkDevice device, uint32_t setCount, std::span<PoolSizeRatio> poolRatios)
{
    std::vector<VkDescriptorPoolSize> poolSizes;
    for (PoolSizeRatio ratio : poolRatios)
    {
        poolSizes.push_back(VkDescriptorPoolSize{
          .type = ratio.type,
          .descriptorCount = static_cast<uint32_t>(ratio.ratio * setCount) });
    }

    VkDescriptorPoolCreateInfo poolInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        // TODO: Make the flag a parameter
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = setCount,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    VkDescriptorPool newPool;
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &newPool);

    return newPool;
}

} // namespace vk
} // namespace baldwin
