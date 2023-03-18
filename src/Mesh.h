#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vulkan/vulkan.hpp>
#include <array>
#include <string>
#include <vector>
#include <memory>

#include "BehVkTypes.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoord;
	uint8_t materialIndex;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && normal == other.normal && texCoord == other.texCoord && materialIndex == other.materialIndex;
	}

	static std::vector<vk::VertexInputBindingDescription> getBindingDescription();

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
};

struct Mesh {
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
	std::vector<std::string> _texturePaths; // TODO: Fix this, it's not a good spot to have it saved.
	std::shared_ptr<AllocatedBuffer> _vertexBuffer;
	std::shared_ptr<AllocatedBuffer> _indexBuffer;

	static std::shared_ptr<Mesh> LoadFromObj(const char* filename);
};