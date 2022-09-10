#pragma once

#include <vulkan/vulkan.hpp>

struct Material {
	vk::DescriptorSet textureSet {};
	vk::PipelineLayout pipelineLayout;
};