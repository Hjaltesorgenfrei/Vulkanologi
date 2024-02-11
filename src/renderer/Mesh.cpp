#include "Mesh.hpp"

#define TINYOBJLOADER_IMPLEMENTATION

#include <tiny_obj_loader.h>

#include <glm/gtx/hash.hpp>
#include <iostream>
#include <string>

namespace std {
template <>
struct hash<Vertex> {
	size_t operator()(Vertex const &vertex) const {
		auto h1 = std::hash<glm::vec3>{}(vertex.pos);
		auto h2 = std::hash<glm::vec3>{}(vertex.color);
		auto h3 = std::hash<glm::vec3>{}(vertex.normal);
		auto h4 = std::hash<glm::vec2>{}(vertex.texCoord);
		auto h5 = std::hash<uint8_t>{}(vertex.materialIndex);
		return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4);  // TODO: Improve this hash function
	}
};
}  // namespace std

std::vector<vk::VertexInputBindingDescription> Vertex::getBindingDescription() {
	return std::vector<vk::VertexInputBindingDescription>{vk::VertexInputBindingDescription{
		.binding = 0, .stride = sizeof(Vertex), .inputRate = vk::VertexInputRate::eVertex}};
}

std::vector<vk::VertexInputAttributeDescription> Vertex::getAttributeDescriptions() {
	std::vector<vk::VertexInputAttributeDescription> attributeDescriptions = {
		vk::VertexInputAttributeDescription{
			.location = 0, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, pos)},
		vk::VertexInputAttributeDescription{
			.location = 1, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, color)},
		vk::VertexInputAttributeDescription{
			.location = 2, .binding = 0, .format = vk::Format::eR32G32B32Sfloat, .offset = offsetof(Vertex, normal)},
		vk::VertexInputAttributeDescription{
			.location = 3, .binding = 0, .format = vk::Format::eR32G32Sfloat, .offset = offsetof(Vertex, texCoord)},
		vk::VertexInputAttributeDescription{
			.location = 4, .binding = 0, .format = vk::Format::eR8Uint, .offset = offsetof(Vertex, materialIndex)},
	};
	return attributeDescriptions;
}

std::shared_ptr<Mesh> Mesh::LoadFromObj(const char *filename) {
	std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	// Get path from filename
	std::string fileDir = filename;
	size_t lastSlash = fileDir.find_last_of("/\\");
	if (lastSlash != std::string::npos) {
		fileDir = fileDir.substr(0, lastSlash + 1);
	} else {
		fileDir = "./";
	}

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, fileDir.c_str())) {
		throw std::runtime_error("RenderData failed to load!\n" + warn + err);
	}

	if (!warn.empty()) {
		std::cout << "WARN: " << warn << " File: " << filename << std::endl;
	}
	if (!err.empty()) {
		std::cerr << "ERR: " << err << " File: " << filename << std::endl;
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};
	// Loop over shapes
	for (auto &shape : shapes) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
			auto fv = size_t(shape.mesh.num_face_vertices[f]);

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t index = shape.mesh.indices[index_offset + v];

				Vertex vertex{
					.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
							attrib.vertices[3 * index.vertex_index + 2]},
					.color = {1.0f, 1.0f, 1.0f},
					.materialIndex =
						static_cast<uint8_t>(shape.mesh.material_ids[f])  // Index of material which is loaded later
				};

				if (index.normal_index >= 0) {
					vertex.normal = {attrib.normals[3 * index.normal_index + 0],
									 attrib.normals[3 * index.normal_index + 1],
									 attrib.normals[3 * index.normal_index + 2]};
				}

				if (index.texcoord_index >= 0) {
					vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
									   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
				}

				if (uniqueVertices.count(vertex) == 0) {
					uniqueVertices[vertex] = static_cast<uint32_t>(mesh->_vertices.size());
					mesh->_vertices.push_back(vertex);
				}

				mesh->_indices.push_back(uniqueVertices[vertex]);
			}
			index_offset += fv;
		}
	}

	for (int materialIndex = 0; materialIndex < materials.size(); materialIndex++) {
		auto &material = materials[materialIndex];
		for (int i = 0; i < mesh->_vertices.size(); i++) {
			if (mesh->_vertices[i].materialIndex == materialIndex) {
				mesh->_vertices[i].color = {material.diffuse[0], material.diffuse[1], material.diffuse[2]};
			}
		}
		if (material.diffuse_texname.empty()) {
			continue;
		}
		mesh->_texturePaths.push_back(fileDir + material.diffuse_texname);
	}

	return mesh;
}