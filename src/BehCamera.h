#pragma once

#include <glm/glm.hpp>


class BehCamera {
public:
    BehCamera();

    glm::mat4 viewMatrix();

    void moveCameraForward(float speed);

    void moveCameraBackward(float speed);

    void moveCameraLeft(float speed);

    void moveCameraRight(float speed);

    void setCameraFront();

    void resetCursorPos();

    void newCursorPos(float xPos, float yPos);

private:

    glm::vec3 cameraPosition;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float cameraYaw;
    float cameraPitch;

    float cursorXPos, cursorYPos;
    bool firstCursorCall = true; // Used to not move the camera far when the mouse enter the screen;
};