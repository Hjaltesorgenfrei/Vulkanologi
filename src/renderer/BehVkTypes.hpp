#pragma once
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct AllocatedBuffer {
	vk::Buffer _buffer;
	VmaAllocation _allocation;
};

template <typename T>
struct PersistentlyMappedBuffer : AllocatedBuffer {
	T* _data;
	size_t _count;

	inline T& at(size_t i) {
		if (i > _count) {
			throw std::out_of_range("PersistentlyMappedBuffer index out of range");
		}
		return _data[i];
	}

	T& operator[](size_t i) { at(i); }
};

struct AllocatedImage {
	vk::Image _image;
	vk::ImageLayout _layout;
	VmaAllocation _allocation;
};

struct MeshPushConstants {
	glm::mat4 model;
	glm::vec4 color = glm::vec4(1.0f);
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
	int width;
	int height;
	uint32_t layerCount = 1;
};