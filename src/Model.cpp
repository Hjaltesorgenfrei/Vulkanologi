#include "Model.h"
#include <chrono>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Model::Model() {
    loadModel();
    modelMatrix = glm::mat4(1.0f);

    cameraYaw   = 135.0f;
    cameraPitch = -35.0f;
    cameraPosition = glm::vec3(2.0f, 2.0f, -2.0f);
    cameraUp       = glm::vec3(0.0f, 1.0f, 0.0f);
    setCameraFront();
}

std::vector<Mesh*> Model::getMeshes() {
    return std::vector<Mesh*>{&mesh};
}

void Model::loadModel() {
    mesh = Mesh::LoadFromObj("resources/viking_room_fixed.obj");
}

MeshPushConstants Model::getPushConstants() {
    return {
            .model = modelMatrix
    };
}

const UniformBufferObject Model::getCameraProject(float width, float height) {
    glm::vec3 cameraDirection = glm::normalize(cameraPosition - glm::vec3(0.0f, 0.0f, 0.0f));
	UniformBufferObject ubo {
		.view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp),
		.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f)
	};

	ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

    ubo.projView = ubo.proj * ubo.view;

    return ubo;
}

void Model::moveCameraForward(float speed) {
    cameraPosition += speed * cameraFront;
}

void Model::moveCameraBackward(float speed) {
    cameraPosition -= speed * cameraFront;
}

void Model::moveCameraLeft(float speed) {
    cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void Model::moveCameraRight(float speed) {
    cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void Model::newCursorPos(float xPos, float yPos) {
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

void Model::setCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    cameraFront = glm::normalize(front);
}

void Model::resetCursorPos() {
    firstCursorCall = true;
}

