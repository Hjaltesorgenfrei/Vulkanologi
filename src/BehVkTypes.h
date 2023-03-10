#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

struct AllocatedBuffer {
    vk::Buffer _buffer; 
    VmaAllocation _allocation;
};

template <typename T>
struct PersistentBuffer : AllocatedBuffer {
    T* _data;
    size_t _count;

    inline T& at(size_t i) {
        if (i > _count) {
            throw std::out_of_range("PersistentBuffer index out of range");
        }
        return _data[i];
    }

    T& operator [](size_t i) {
        at(i);
    }
};

struct AllocatedImage {
    vk::Image _image;
    VmaAllocation _allocation;
};

struct MeshPushConstants {
    glm::mat4 model;
};

struct UploadContext {
    vk::Fence _uploadFence;
    vk::CommandPool _commandPool;
    vk::CommandBuffer _commandBuffer;
};

struct UploadedTexture {
	uint32_t mipLevels;
    AllocatedImage textureImage;
    vk::DeviceMemory textureImageMemory;
	vk::ImageView textureImageView;
    vk::Format textureImageFormat;
	vk::Sampler textureSampler;
};