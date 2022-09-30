// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include "VkTypes.h"
#include <vector>
#include <array>
#include <unordered_map>
#include <optional>

// Shamelessly copied from https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.h

class DescriptorAllocator {
public:

    struct PoolSizes {
        std::vector<std::pair<vk::DescriptorType, float>> sizes =
                {
                        {vk::DescriptorType::eSampler,              0.5f},
                        {vk::DescriptorType::eCombinedImageSampler, 4.0f},
                        {vk::DescriptorType::eSampledImage,         4.0f},
                        {vk::DescriptorType::eStorageImage,         1.0f},
                        {vk::DescriptorType::eUniformTexelBuffer,   1.0f},
                        {vk::DescriptorType::eStorageTexelBuffer,   1.0f},
                        {vk::DescriptorType::eUniformBuffer,        2.0f},
                        {vk::DescriptorType::eStorageBuffer,        2.0f},
                        {vk::DescriptorType::eUniformBufferDynamic, 1.0f},
                        {vk::DescriptorType::eStorageBufferDynamic, 1.0f},
                        {vk::DescriptorType::eInputAttachment,      0.5f}
                };
    };

    void resetPools();

    bool allocate(vk::DescriptorSet *set, vk::DescriptorSetLayout layout, const std::vector<uint32_t>& counts = std::vector<uint32_t>());

    void init(vk::Device newDevice);

    void cleanup();

    vk::Device device;
private:
    vk::DescriptorPool grabPool();

    std::optional<vk::DescriptorPool> currentPool;
    PoolSizes descriptorSizes;
    std::vector<vk::DescriptorPool> usedPools;
    std::vector<vk::DescriptorPool> freePools;
};


class DescriptorLayoutCache {
public:
    void init(vk::Device newDevice);

    void cleanup();

    vk::DescriptorSetLayout createDescriptorLayout(vk::DescriptorSetLayoutCreateInfo *info);

    struct DescriptorLayoutInfo {
        //good idea to turn this into a inlined array
        std::vector<vk::DescriptorSetLayoutBinding> bindings;

        bool operator==(const DescriptorLayoutInfo &other) const;

        size_t hash() const;
    };


private:

    struct DescriptorLayoutHash {

        std::size_t operator()(const DescriptorLayoutInfo &k) const {
            return k.hash();
        }
    };

    std::unordered_map<DescriptorLayoutInfo, vk::DescriptorSetLayout, DescriptorLayoutHash> layoutCache;
    vk::Device device;
};


class DescriptorBuilder {
public:

    static DescriptorBuilder begin(DescriptorLayoutCache *layoutCache, DescriptorAllocator *allocator);

    DescriptorBuilder &bindBuffer(uint32_t binding, vk::DescriptorBufferInfo *bufferInfo, vk::DescriptorType type,
                                   vk::ShaderStageFlags stageFlags);

    DescriptorBuilder &bindImage(uint32_t binding, vk::DescriptorImageInfo *imageInfo, vk::DescriptorType type,
                                  vk::ShaderStageFlags stageFlags);

    DescriptorBuilder &bindImages(uint32_t binding, std::vector<vk::DescriptorImageInfo>& imageInfos, vk::DescriptorType type,
                                 vk::ShaderStageFlags stageFlags);

    bool build(vk::DescriptorSet &set, vk::DescriptorSetLayout &layout);

    bool build(vk::DescriptorSet &set);

private:

    std::vector<vk::WriteDescriptorSet> writes;
    std::vector<vk::DescriptorSetLayoutBinding> bindings;


    DescriptorLayoutCache *cache;
    DescriptorAllocator *alloc;
};

