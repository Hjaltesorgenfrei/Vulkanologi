#include "BehDescriptors.h"
#include <algorithm>
#include <iostream>

// Shamelessly inspired by https://github.com/vblanco20-1/vulkan-guide/blob/engine/extra-engine/vk_descriptors.cpp

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

DescriptorSetLayoutBuilder DescriptorSetLayoutBuilder::begin(DescriptorLayoutCache *layoutCache) {
    DescriptorSetLayoutBuilder builder;

    builder.cache = layoutCache;
    return builder;
}

DescriptorSetLayoutBuilder &
DescriptorSetLayoutBuilder::addBinding(uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stageFlags,
                                       uint32_t count, vk::DescriptorBindingFlags flags) {
    vk::DescriptorSetLayoutBinding newBinding{
            .binding = binding,
            .descriptorType = type,
            .descriptorCount = count,
            .stageFlags = stageFlags
    };

    bindings.push_back(newBinding);
    bindingFlags.push_back(flags);

    return *this;
}

bool DescriptorSetLayoutBuilder::build(vk::DescriptorSetLayout &layout) {
#ifndef NDEBUG
    if (!validate()) {
        return false;
    }
#endif

    vk::DescriptorSetLayoutCreateInfo layoutInfo{
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
    };

    bool anyFlags = false;
    std::vector<uint32_t> counts;
    for (size_t i = 0; i < bindings.size(); i++) {
        const auto &binding = bindings[i];
        counts.push_back(binding.descriptorCount);
        const auto &bindingFlag = bindingFlags[i];
        anyFlags = anyFlags || bindingFlag;
    }

    if (anyFlags) {
        vk::DescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCreateInfo{
                .bindingCount = static_cast<uint32_t>(bindingFlags.size()),
                .pBindingFlags = bindingFlags.data()
        };

        layoutInfo.pNext = &bindingFlagsCreateInfo;
    }

    layout = cache->createDescriptorLayout(&layoutInfo);
    return true;
}

bool DescriptorSetLayoutBuilder::validate() {
    bool result = true;

    for (size_t i = 0; i < bindingFlags.size(); i++) {
        const auto &bindingFlag = bindingFlags[i];
        if (bindingFlag & vk::DescriptorBindingFlagBits::eVariableDescriptorCount && i + 1 != bindingFlags.size()) {
            std::cerr << "VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT may only be set on the last descriptor, "
                         "but is set on binding[" << i
                      << "]. VUID-VkDescriptorSetLayoutBindingFlagsCreateInfo-pBindingFlags-03004\n";
            result = false;
        }
    }

    for (size_t i = 0; i < bindings.size(); i++) {
        const auto &binding = bindings[i];
        if (i + 1 < bindings.size() && bindings[i].binding <= binding.binding) {
            std::cerr
                    << "Please add your bindings in order. This check might be removed later if I sort the order later.\n";
            result = false;
        }
    }

    return result;
}

DescriptorSetBuilder DescriptorSetBuilder::begin(vk::DescriptorSetLayout layout, DescriptorAllocator *allocator) {
    DescriptorSetBuilder builder;

    builder.layout = layout;
    builder.allocator = allocator;

    return builder;
}

DescriptorSetBuilder &
DescriptorSetBuilder::bindBuffer(uint32_t binding, vk::DescriptorBufferInfo *bufferInfo, vk::DescriptorType type) {
    vk::WriteDescriptorSet newWrite{
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = type,
            .pBufferInfo = bufferInfo,
    };

    writes.push_back(newWrite);
    return *this;
}

DescriptorSetBuilder &
DescriptorSetBuilder::bindImage(uint32_t binding, vk::DescriptorImageInfo *imageInfo, vk::DescriptorType type) {
    vk::WriteDescriptorSet newWrite{
            .dstBinding = binding,
            .descriptorCount = 1,
            .descriptorType = type,
            .pImageInfo = imageInfo,
    };

    writes.push_back(newWrite);
    return *this;
}

DescriptorSetBuilder &
DescriptorSetBuilder::bindImages(uint32_t binding, std::vector<vk::DescriptorImageInfo> &imageInfos,
                                 vk::DescriptorType type) {
    vk::WriteDescriptorSet newWrite{
            .dstBinding = binding,
            .descriptorCount = static_cast<uint32_t>(imageInfos.size()),
            .descriptorType = type,
            .pImageInfo = imageInfos.data()
    };

    writes.push_back(newWrite);
    return *this;
}

bool DescriptorSetBuilder::build(vk::DescriptorSet &set) {
    std::vector<uint32_t> counts;
    for (const auto &write: writes) {
        counts.push_back(write.descriptorCount);
    }

    bool success = std::all_of(writes.cbegin(), writes.cend(),
                               [](vk::WriteDescriptorSet write) { return write.descriptorCount > 1; })
                   ? allocator->allocate(&set, layout, counts)
                   : allocator->allocate(&set, layout);

    if (!success) { return false; }

    for (vk::WriteDescriptorSet &w: writes) {
        w.dstSet = set;
    }

    allocator->device.updateDescriptorSets(static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);

    return true;
}
