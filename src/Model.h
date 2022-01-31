#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Vertice.h"

struct MeshPushConstants {
    glm::vec4 model;
};

struct UniformBufferObject
{
    glm::mat4 view;
    glm::mat4 proj;
};

class Model
{
public:
    const std::vector<Vertex> getVertices();
    const std::vector<uint32_t> getIndices();
    const MeshPushConstants getPushConstants();
    const UniformBufferObject getCameraProject(float width, float height);
    void moveCameraForward(float speed);
    void moveCameraBackward(float speed);
    void moveCameraLeft(float speed);
    void moveCameraRight(float speed);
    void resetCursorPos();
    void newCursorPos(float xPos, float yPos);
    Model();

private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

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