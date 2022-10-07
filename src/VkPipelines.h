#include "VulkanDevice.h"
#include "Deletionqueue.h"

#pragma once

class VkPipelineBuilder {
public:
	static VkPipelineBuilder begin(VulkanDevice* device, vk::Extent2D extent, vk::PipelineLayout pipelineLayout, vk::RenderPass renderPass);
	VkPipelineBuilder &polygonMode(vk::PolygonMode polygonMode);
	VkPipelineBuilder &shader(const std::string& filepath, vk::ShaderStageFlagBits stage);
	bool build(DeletionQueue& deletionQueue, vk::Pipeline& pipeline);

private:
	VulkanDevice* device;
	vk::Extent2D extent;
	vk::PipelineLayout pipelineLayout;
	vk::RenderPass renderPass;

	// Fields that can be changed, possibly use optional at some point if it is necessary
	vk::PolygonMode _polygonMode = vk::PolygonMode::eFill;
	std::vector<std::pair<std::string, vk::ShaderStageFlagBits>> _shaders;

	vk::ShaderModule createShaderModule(const std::vector<char> &code);
};
