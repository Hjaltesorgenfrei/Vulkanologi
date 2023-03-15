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

glm::mat4 BehCamera::viewMatrix() const {
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

glm::vec3 BehCamera::getCameraPosition() const
{
    return cameraPosition;
}

glm::vec3 BehCamera::getRayDirection(float xPos, float yPos, float width, float height) const
{
    glm::vec4 rayClip = glm::vec4((2.0f * xPos) / width - 1.0f, 1.0f - (2.0f * yPos) / height, -1.0f, 1.0f);
    auto proj = getCameraProjection(width, height);
    proj[1][1] *= -1; // Flip it back to the original orientation, as I am doing math I don't understand.
    // TODO: Figure out why this is necessary. And learn math.
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(viewMatrix()) * rayEye));
    return rayWorld;
}

void BehCamera::resetCursorPos() {
    firstCursorCall = true;
}

glm::mat4 BehCamera::getCameraProjection(float width, float height) const {
    auto proj = glm::perspective(glm::radians(fovY), width / height, 0.1f, 1000.0f);
    proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.
    return proj;
}