#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "BehCamera.h"

BehCamera::BehCamera() {
    cameraYaw = 135.0f;
    cameraPitch = -35.0f;
    cameraPosition = glm::vec3(2.0f, 2.0f, -2.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    setCameraFront();
}

glm::mat4 BehCamera::viewMatrix() {
    return glm::lookAt(cameraPosition, cameraPosition + cameraFront, cameraUp);
}

void BehCamera::setCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    cameraFront = glm::normalize(front);
}

void BehCamera::moveCameraForward(float speed) {
    cameraPosition += speed * cameraFront;
}

void BehCamera::moveCameraBackward(float speed) {
    cameraPosition -= speed * cameraFront;
}

void BehCamera::moveCameraLeft(float speed) {
    cameraPosition -= glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}

void BehCamera::moveCameraRight(float speed) {
    cameraPosition += glm::normalize(glm::cross(cameraFront, cameraUp)) * speed;
}


void BehCamera::newCursorPos(float xPos, float yPos) {
    if (firstCursorCall) {
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

void BehCamera::resetCursorPos() {
    firstCursorCall = true;
}
