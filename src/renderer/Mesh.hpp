#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "BehVkTypes.hpp"

struct CarSettings;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoord;
	uint8_t materialIndex;

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && normal == other.normal && texCoord == other.texCoord &&
			   materialIndex == other.materialIndex;
	}

	static std::vector<vk::VertexInputBindingDescription> getBindingDescription();

	static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
};

struct Mesh {
	std::vector<Vertex> _vertices;
	std::vector<uint32_t> _indices;
	std::shared_ptr<AllocatedBuffer> _vertexBuffer;
	std::shared_ptr<AllocatedBuffer> _indexBuffer;

	// TODO: Move to somesort of OBJ loader
	[[nodiscard]] static std::shared_ptr<Mesh> LoadFromObj(std::string filename);
	[[nodiscard]] static std::vector<std::string> MaterialPathsFromObj(std::string filename);
};
