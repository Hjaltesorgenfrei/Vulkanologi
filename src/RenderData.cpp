#include "RenderData.h"
#include <chrono>
#include <unordered_map>
#include "Mesh.h"
#include "Material.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

RenderData::RenderData() {
    loadModel();
}

RenderData::~RenderData() {
    for (auto model : models) {
        delete model;
    }
}

std::vector<RenderObject*> RenderData::getModels() {
    return models;
}

void RenderData::loadModel() {
    models.push_back(new RenderObject(Mesh::LoadFromObj("resources/lost_empire.obj"), Material{}));
    models.push_back(new RenderObject(Mesh::LoadFromObj("resources/rat.obj"), Material{}));
}

const UniformBufferObject RenderData::getCameraProject(float width, float height) {
	UniformBufferObject ubo {
        .view = camera.viewMatrix(),
		.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f)
	};

	ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

    ubo.projView = ubo.proj * ubo.view;

    return ubo;
}
