#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Version 145 at least
#include <vulkan/vulkan.hpp>
#include <array>
#include <string>
#include <vector>

#include "VkTypes.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoord;
	uint8_t materialIndex;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}

	static vk::VertexInputBindingDescription getBindingDescription();

	static std::array<vk::VertexInputAttributeDescription, 5> getAttributeDescriptions();
};

struct Mesh {
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
	std::vector<std::string> _texturePaths; // TODO: Fix this, it's not a good spot to have it saved.
	AllocatedBuffer _vertexBuffer;
	AllocatedBuffer _indexBuffer;

	static Mesh LoadFromObj(const char* filename);
};