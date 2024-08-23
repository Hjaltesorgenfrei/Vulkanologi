#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>
#include "BehVkTypes.hpp"

struct MaterialData {
	glm::vec4 color = glm::vec4(1.0f);
};

struct Material {
	std::vector<MaterialData> data;
	std::shared_ptr<AllocatedBuffer> uniformBuffer;
	vk::DescriptorSet textureSet;
};