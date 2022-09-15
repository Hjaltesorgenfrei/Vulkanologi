#pragma once

#include <vulkan/vulkan.hpp>
#include <map>
#include <vector>

struct AnnotatedDescriptorSetLayout {
    vk::DescriptorSetLayout layout;
    vk::DescriptorType type;
};

class DescriptorSetManager {
    DescriptorSetManager(vk::Device device);
    ~DescriptorSetManager();
    DescriptorSetManager& operator=(const DescriptorSetManager&) = delete;
	DescriptorSetManager(const DescriptorSetManager&) = delete;
    bool allocate(vk::DescriptorSet* descriptorSet, AnnotatedDescriptorSetLayout layout);

private:
    vk::Device device;
    std::vector<vk::DescriptorPool> usedPools;
    std::map<vk::DescriptorType, vk::DescriptorPool> freePools;

    vk::DescriptorPool getPool(vk::DescriptorType type);
    vk::DescriptorPool createPool(vk::DescriptorType type);
};