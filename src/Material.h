#pragma once

#include <vulkan/vulkan.hpp>

struct Material {
	const char* filename;

	vk::DescriptorSet textureSet {};
	vk::Pipeline pipeline;
	vk::PipelineLayout pipelineLayout;

	static Material LoadFromPng(const char* filename);
};