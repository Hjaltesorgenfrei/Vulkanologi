#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Version 145 at least
#include <vulkan/vulkan.hpp>
#include <array>
#include <vector>

#include "VkTypes.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}

	static vk::VertexInputBindingDescription getBindingDescription();

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
};

struct Mesh {
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
	AllocatedBuffer _vertexBuffer;
	AllocatedBuffer _indexBuffer;

	static Mesh LoadFromObj(const char* filename);
};