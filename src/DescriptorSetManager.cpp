#include "DescriptorSetManager.h"

DescriptorSetManager::DescriptorSetManager(vk::Device device) {
    this->device = device;
}

DescriptorSetManager::~DescriptorSetManager() {
    for (auto descriptorPool : usedPools) {
        device.destroyDescriptorPool(descriptorPool, nullptr);
    }

    for (auto const& value : freePools) {
        device.destroyDescriptorPool(value.second, nullptr);
    }
}

bool DescriptorSetManager::allocate(vk::DescriptorSet* descriptorSet, AnnotatedDescriptorSetLayout layout) {
    auto pool = getPool(layout.type);
    return false;
}

vk::DescriptorPool DescriptorSetManager::getPool(vk::DescriptorType type) {
    auto it = freePools.find(type);
    if (it != freePools.end()) {
        auto pool = it->second;
        freePools.erase(it);
        return pool;
    }
    return createPool(type);
}

vk::DescriptorPool DescriptorSetManager::createPool(vk::DescriptorType type) {
    return vk::DescriptorPool{};
}
