#pragma once

#include <vulkan/vulkan.hpp>

struct Material {
	const char* filename;

	vk::DescriptorSet textureSet {};
	vk::PipelineLayout pipelineLayout;
};