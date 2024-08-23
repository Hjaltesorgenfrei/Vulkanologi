#include "Cube.hpp"

const std::vector<glm::vec3> cubePositions = {{-0.5f, -0.5f, -0.5f}, {0.5f, -0.5f, -0.5f}, {0.5f, 0.5f, -0.5f},
											  {-0.5f, 0.5f, -0.5f},  {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f},
											  {0.5f, 0.5f, 0.5f},    {-0.5f, 0.5f, 0.5f}};

const std::vector<glm::vec3> cubeNormals = {{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 1.0f},  {0.0f, -1.0f, 0.0f},
											{0.0f, 1.0f, 0.0f},  {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};

const std::vector<glm::vec2> texCoords = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

std::shared_ptr<Mesh> createCubeMesh(std::string texturePath) {
	auto mesh = std::make_shared<Mesh>();

	std::vector<Vertex> vertices = {{cubePositions[2], cubeNormals[0], texCoords[0], 0},
									{cubePositions[1], cubeNormals[0], texCoords[3], 0},
									{cubePositions[3], cubeNormals[0], texCoords[1], 0},
									{cubePositions[0], cubeNormals[0], texCoords[2], 0},

									{cubePositions[4], cubeNormals[1], texCoords[3], 0},
									{cubePositions[5], cubeNormals[1], texCoords[2], 0},
									{cubePositions[7], cubeNormals[1], texCoords[0], 0},
									{cubePositions[6], cubeNormals[1], texCoords[1], 0},

									{cubePositions[0], cubeNormals[4], texCoords[3], 0},
									{cubePositions[3], cubeNormals[4], texCoords[0], 0},
									{cubePositions[7], cubeNormals[4], texCoords[1], 0},
									{cubePositions[4], cubeNormals[4], texCoords[2], 0},

									{cubePositions[1], cubeNormals[5], texCoords[2], 0},
									{cubePositions[2], cubeNormals[5], texCoords[1], 0},
									{cubePositions[6], cubeNormals[5], texCoords[0], 0},
									{cubePositions[5], cubeNormals[5], texCoords[3], 0},

									{cubePositions[3], cubeNormals[3], texCoords[1], 0},
									{cubePositions[2], cubeNormals[3], texCoords[0], 0},
									{cubePositions[6], cubeNormals[3], texCoords[3], 0},
									{cubePositions[7], cubeNormals[3], texCoords[2], 0},

									{cubePositions[0], cubeNormals[2], texCoords[0], 0},
									{cubePositions[1], cubeNormals[2], texCoords[1], 0},
									{cubePositions[5], cubeNormals[2], texCoords[2], 0},
									{cubePositions[4], cubeNormals[2], texCoords[3], 0}};

	std::vector<uint32_t> indices = {0,  1,  2,  1,  3,  2,

									 4,  5,  6,  5,  7,  6,

									 9,  8,  11, 9,  11, 10,

									 12, 13, 14, 15, 12, 14,

									 16, 19, 17, 17, 19, 18,

									 20, 21, 22, 23, 20, 22};

	mesh->_vertices = vertices;
	mesh->_indices = indices;

	return mesh;
}