#include "Model.h"
#include <chrono>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Model::Model() {
    vertices = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}}
    };

    indices = {0, 1, 2, 2, 3, 0};

    cameraPosition = glm::vec3(0.0f, 0.0f, 2.0f);
    cameraFront    = glm::vec3(0.0f, 0.0f, -1.0f);
    cameraUp       = glm::vec3(0.0f, 1.0f, 0.0f);
}

const std::vector<Vertex> Model::getVertices() {
    return vertices;
}
const std::vector<uint16_t> Model::getIndices() {
    return indices;
}

const UniformBufferObject Model::getCameraProject(float width, float height) {
    glm::vec3 cameraDirection = glm::normalize(cameraPosition - glm::vec3(0.0f, 0.0f, 0.0f));
	UniformBufferObject ubo {
		.model = glm::mat4(1.0f),
		.view = glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp),
		.proj = glm::perspective(glm::radians(45.0f), width / height, 0.1f, 1000.0f)
	};

	ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

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
    }

    float xOffset = xPos - cursorXPos;
    float yOffset = cursorYPos - yPos; // reversed since y-coordinates go from bottom to top
    cursorXPos = xPos;
    cursorYPos = yPos;

    float sensitivity = 0.2f; // change this value to your liking
    xOffset *= sensitivity;
    yOffset *= sensitivity;

    cameraYaw += xOffset;
    cameraPitch += yOffset;

    // make sure that when cameraPitch is out of bounds, screen doesn't get flipped
    if (cameraPitch > 89.0f)
        cameraPitch = 89.0f;
    if (cameraPitch < -89.0f)
        cameraPitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    cameraFront = glm::normalize(front);
}

void Model::resetCursorPos() {
    firstCursorCall = true;
}

