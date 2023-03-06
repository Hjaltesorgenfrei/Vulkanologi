#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

struct AllocatedBuffer {
    vk::Buffer _buffer; 
    VmaAllocation _allocation;
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