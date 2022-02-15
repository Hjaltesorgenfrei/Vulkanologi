#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Mesh.h"

struct MeshPushConstants {
    glm::vec4 model;
};

struct UniformBufferObject
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projView;
};

class Model
{
public:
    const MeshPushConstants getPushConstants();
    const UniformBufferObject getCameraProject(float width, float height);
    std::vector<Mesh*> getMeshes();
    void moveCameraForward(float speed);
    void moveCameraBackward(float speed);
    void moveCameraLeft(float speed);
    void moveCameraRight(float speed);
    void resetCursorPos();
    void newCursorPos(float xPos, float yPos);
    Model();

private:
    Mesh mesh;

    glm::vec3 cameraPosition;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;
    float cursorXPos, cursorYPos;
    float cameraYaw;
    float cameraPitch;
    bool firstCursorCall = true;

    void loadModel();
    void setCameraFront();
};