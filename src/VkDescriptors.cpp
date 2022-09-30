#include "VkDescriptors.h"
#include <algorithm>
#include <iostream>

// Shamelessly copied from https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.cpp

vk::DescriptorPool createPool(vk::Device device, const DescriptorAllocator::PoolSizes &poolSizes, int count,
                              vk::DescriptorPoolCreateFlags flags) {
    std::vector<vk::DescriptorPoolSize> sizes;
    sizes.reserve(poolSizes.sizes.size());
    for (auto sz: poolSizes.sizes) {
        sizes.push_back({sz.first, uint32_t(sz.second * count)});
    }
    vk::DescriptorPoolCreateInfo poolInfo = {
            .flags = flags,
            .maxSets = static_cast<uint32_t>(count),
            .poolSizeCount = (uint32_t) sizes.size(),
            .pPoolSizes = sizes.data()
    };

    return device.createDescriptorPool(poolInfo);
}

void DescriptorAllocator::resetPools() {
    for (auto p: usedPools) {
        vkResetDescriptorPool(device, p, 0);
    }

    freePools = usedPools;
    usedPools.clear();
    currentPool = VK_NULL_HANDLE;
}

bool DescriptorAllocator::allocate(vk::DescriptorSet *set, vk::DescriptorSetLayout layout,
                                   const std::vector<uint32_t> &counts) {
    using vk::Result;

    if (!currentPool.has_value()) {
        currentPool = grabPool();
        usedPools.push_back(currentPool.value());
    }


    vk::DescriptorSetAllocateInfo allocInfo = {
            .descriptorPool = currentPool.value(),
            .descriptorSetCount = 1,
            .pSetLayouts = &layout,
    };

    if (!counts.empty()) {
        vk::DescriptorSetVariableDescriptorCountAllocateInfo setCounts = {
                .descriptorSetCount = 1,
                .pDescriptorCounts = counts.data()
        };

        allocInfo.pNext = &setCounts;
    }

    auto allocResult = device.allocateDescriptorSets(&allocInfo, set);
    bool needReallocate = false;

    switch (allocResult) {
        case Result::eSuccess:
            //all good, return
            return true;
            break;

        case Result::eErrorFragmentedPool:
        case Result::eErrorOutOfPoolMemory:
            //reallocate pool
            needReallocate = true;
            break;

        default:
            //unrecoverable error
            return false;
    }

    if (needReallocate) {
        // Allocate a new pool and retry
        currentPool = grabPool();
        usedPools.push_back(currentPool.value());
        allocInfo.descriptorPool = currentPool.value();

        auto allocRetryResult = device.allocateDescriptorSets(&allocInfo, set);

        // If it still fails then we have big issues
        // Probably need to
        if (allocRetryResult == Result::eSuccess) {
            return true;
        } else {
            std::cerr << "Tried to allocate descriptorset but failed after creating new pool" << std::endl;
        }
    }

    return false;
}

void DescriptorAllocator::init(vk::Device newDevice) {
    device = newDevice;
}

void DescriptorAllocator::cleanup() {
    //delete every pool held
    for (auto p: freePools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
    for (auto p: usedPools) {
        vkDestroyDescriptorPool(device, p, nullptr);
    }
}

vk::DescriptorPool DescriptorAllocator::grabPool() {
    if (!freePools.empty()) {
        vk::DescriptorPool pool = freePools.back();
        freePools.pop_back();
        return pool;
    } else {
        return createPool(device, descriptorSizes, 1000, vk::DescriptorPoolCreateFlags());
    }
}


void DescriptorLayoutCache::init(vk::Device newDevice) {
    device = newDevice;
}

vk::DescriptorSetLayout DescriptorLayoutCache::createDescriptorLayout(vk::DescriptorSetLayoutCreateInfo *info) {
    DescriptorLayoutInfo layoutInfo;
    layoutInfo.bindings.reserve(info->bindingCount);
    bool isSorted = true;
    int32_t lastBinding = -1;
    for (uint32_t i = 0; i < info->bindingCount; i++) {
        layoutInfo.bindings.push_back(info->pBindings[i]);

        //check that the bindings are in strict increasing order
        if (static_cast<int32_t>(info->pBindings[i].binding) > lastBinding) {
            lastBinding = static_cast<int32_t>(info->pBindings[i].binding);
        } else {
            isSorted = false;
        }
    }
    if (!isSorted) {
        std::sort(layoutInfo.bindings.begin(), layoutInfo.bindings.end(),
                  [](vk::DescriptorSetLayoutBinding &a, vk::DescriptorSetLayoutBinding &b) {
                      return a.binding < b.binding;
                  });
    }

    auto it = layoutCache.find(layoutInfo);
    if (it != layoutCache.end()) {
        return (*it).second;
    } else {
        vk::DescriptorSetLayout layout;
        auto result = device.createDescriptorSetLayout(info, nullptr, &layout);

        layoutCache[layoutInfo] = layout;
        return layout;
    }
}


void DescriptorLayoutCache::cleanup() {
    //delete every descriptor layout held
    for (const auto &pair: layoutCache) {
        vkDestroyDescriptorSetLayout(device, pair.second, nullptr);
    }
}

DescriptorBuilder DescriptorBuilder::begin(DescriptorLayoutCache *layoutCache, DescriptorAllocator *allocator) {
    DescriptorBuilder builder;

    builder.cache = layoutCache;
    builder.alloc = allocator;
    return builder;
}


DescriptorBuilder &
DescriptorBuilder::bindBuffer(uint32_t binding, vk::DescriptorBufferInfo *bufferInfo, vk::DescriptorType type,
                              vk::ShaderStageFlags stageFlags) {
    vk::DescriptorSetLayoutBinding newBinding{
            .binding = binding,
            .descriptorType = type,
            .descriptorCount = 1,
            .stageFlags = stageFlags
    };

    bindings.push_back(newBinding);

    vk::WriteDescriptorSet newWrite{
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = type,
            .pBufferInfo = bufferInfo,
    };

    writes.push_back(newWrite);
    return *this;
}


DescriptorBuilder &
DescriptorBuilder::bindImage(uint32_t binding, vk::DescriptorImageInfo *imageInfo, vk::DescriptorType type,
                             vk::ShaderStageFlags stageFlags) {
    vk::DescriptorSetLayoutBinding newBinding{
            .binding = binding,
            .descriptorType = type,
            .descriptorCount = 1,
            .stageFlags = stageFlags
    };

    bindings.push_back(newBinding);

    vk::WriteDescriptorSet newWrite{
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = type,
            .pImageInfo = imageInfo,
    };

    writes.push_back(newWrite);
    return *this;
}

DescriptorBuilder &
DescriptorBuilder::bindImages(uint32_t binding, std::vector<vk::DescriptorImageInfo> &imageInfos,
                              vk::DescriptorType type,
                              vk::ShaderStageFlags stageFlags) {

    // We take the power of two so caching is possible
    vk::DescriptorSetLayoutBinding newBinding{
            .binding = binding,
            .descriptorType = type,
            .descriptorCount = 256,
            .stageFlags = stageFlags
    };

    bindings.push_back(newBinding);

    vk::WriteDescriptorSet newWrite{
            .dstBinding = binding,
            .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
            .descriptorType = type,
            .pImageInfo = imageInfos.data()
    };

    writes.push_back(newWrite);
    return *this;
}

bool DescriptorBuilder::build(vk::DescriptorSet &set, vk::DescriptorSetLayout &layout) {
    //build layout first
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
    };

    bool anyPartialBound = false;
    std::vector<uint32_t> counts;
    for (const auto &binding: bindings) {
        if (binding.descriptorCount > 1 && !anyPartialBound) {
            std::array<vk::DescriptorBindingFlags, 1> flags = {vk::DescriptorBindingFlagBits::eVariableDescriptorCount |
                                                               vk::DescriptorBindingFlagBits::ePartiallyBound};

            vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{
                    .bindingCount = static_cast<uint32_t>(bindings.size()),
                    .pBindingFlags = flags.data()
            };

            layoutInfo.pNext = &bindingFlags;
            anyPartialBound = true;
        }
        counts.push_back(binding.descriptorCount);
    }

    layout = cache->createDescriptorLayout(&layoutInfo);

    //allocate descriptor
    bool success = anyPartialBound ? alloc->allocate(&set, layout, counts) : alloc->allocate(&set, layout);
    if (!success) { return false; }

    //write descriptor

    for (vk::WriteDescriptorSet &w: writes) {
        w.dstSet = set;
    }

    alloc->device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    return true;
}

bool DescriptorBuilder::build(vk::DescriptorSet &set) {
    vk::DescriptorSetLayout layout;
    return build(set, layout);
}


bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(const DescriptorLayoutInfo &other) const {
    if (other.bindings.size() != bindings.size()) {
        return false;
    } else {
        //compare each of the bindings is the same. Bindings are sorted so they will match
        for (int i = 0; i < bindings.size(); i++) {
            if (other.bindings[i].binding != bindings[i].binding) {
                return false;
            }
            if (other.bindings[i].descriptorType != bindings[i].descriptorType) {
                return false;
            }
            if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) {
                return false;
            }
            if (other.bindings[i].stageFlags != bindings[i].stageFlags) {
                return false;
            }
        }
        return true;
    }
}

size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const {
    using std::size_t;
    using std::hash;

    size_t result = hash<size_t>()(bindings.size());

    for (const vk::DescriptorSetLayoutBinding &b: bindings) {
        //pack the binding data into a single int64. Not fully correct but it's ok
        size_t binding_hash = b.binding | static_cast<size_t>(b.descriptorType) << 8 | b.descriptorCount << 16 |
                              static_cast<unsigned int>(b.stageFlags) << 24;

        //shuffle the packed binding data and xor it with the main hash
        result ^= hash<size_t>()(binding_hash);
    }

    return result;
}

