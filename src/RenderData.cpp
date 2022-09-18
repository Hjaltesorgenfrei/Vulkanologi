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
    modelMatrix = glm::mat4(1.0f);

    cameraYaw   = 135.0f;
    cameraPitch = -35.0f;
    cameraPosition = glm::vec3(2.0f, 2.0f, -2.0f);
    cameraUp       = glm::vec3(0.0f, 1.0f, 0.0f);
    setCameraFront();
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
    auto mesh = Mesh::LoadFromObj("resources/lost_empire.obj");
    auto texture = Material{};
    RenderObject* model = new RenderObject(mesh, texture);
    models.push_back(model);
}

MeshPushConstants RenderData::getPushConstants() {
    return {
            .model = modelMatrix
    };
}

const UniformBufferObject RenderData::getCameraProject(float width, float height) {
    glm::vec3 cameraDirection = glm::normalize(cameraPosition - glm::vec3(0.0f, 0.0f, 0.0f));
	UniformBufferObject ubo {
		.view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp),
		.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f)
	};

	ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

    ubo.projView = ubo.proj * ubo.view;

    return ubo;
}

void RenderData::moveCameraForward(float speed) {
    cameraPosition += speed * cameraFront;
}

void RenderData::moveCameraBackward(float speed) {
    cameraPosition -= speed * cameraFront;
}

void RenderData::moveCameraLeft(float speed) {
    cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void RenderData::moveCameraRight(float speed) {
    cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void RenderData::newCursorPos(float xPos, float yPos) {
    if (firstCursorCall)
    {
        cursorXPos = xPos;
        cursorYPos = yPos;
        firstCursorCall = false;
        return;
    }

    float xOffset = xPos - cursorXPos;
    float yOffset = cursorYPos - yPos; // reversed since y-coordinates go from bottom to top
    cursorXPos = xPos;
    cursorYPos = yPos;

    float sensitivity = 0.2f;
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    cameraYaw += xOffset;
    cameraPitch += yOffset;

    // make sure that when cameraPitch is out of bounds, screen doesn't get flipped
    if (cameraPitch > 89.0f)
        cameraPitch = 89.0f;
    if (cameraPitch < -89.0f)
        cameraPitch = -89.0f;
        
    setCameraFront();
}

void RenderData::setCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    cameraFront = glm::normalize(front);
}

void RenderData::resetCursorPos() {
    firstCursorCall = true;
}

