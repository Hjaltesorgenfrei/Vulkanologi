#include "Model.h"
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Model::Model() {
    vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
        {{1.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{1.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}};

     indices = {0, 1, 2, 2, 3, 0, 4, 5, 6};
}

const std::vector<Vertex> Model::getVertices() {
    return vertices;
}
const std::vector<uint16_t> Model::getIndices() {
    return indices;
}

const UniformBufferObject Model::getCameraProject(float width, float height) {
    static auto startTime = std::chrono::high_resolution_clock::now();

	const auto currentTime = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo {
		.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 10.0f)
	};

	ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

    return ubo;
}