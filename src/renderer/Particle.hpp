#pragma once
#ifndef VULKANOLOGI_PARTICLE_H
#define VULKANOLOGI_PARTICLE_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct Particle {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec4 color;

	static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions() {
		return {vk::VertexInputBindingDescription{
			.binding = 0, .stride = sizeof(Particle), .inputRate = vk::VertexInputRate::eVertex}};
	}

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
		return {
			vk::VertexInputAttributeDescription{.location = 0,
												.binding = 0,
												.format = vk::Format::eR32G32Sfloat,
												.offset = offsetof(Particle, position)},
			vk::VertexInputAttributeDescription{.location = 1,
												.binding = 0,
												.format = vk::Format::eR32G32B32A32Sfloat,
												.offset = offsetof(Particle, color)},
		};
	}
};

#endif  // VULKANOLOGI_PARTICLE_H
