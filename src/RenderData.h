#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "Model.h"

struct UniformBufferObject
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projView;
};

class RenderData
{
public:
    MeshPushConstants getPushConstants();
    const UniformBufferObject getCameraProject(float width, float height);
    std::vector<std::shared_ptr<Model>> getModels();
    void moveCameraForward(float speed);
    void moveCameraBackward(float speed);
    void moveCameraLeft(float speed);
    void moveCameraRight(float speed);
    void resetCursorPos();
    void newCursorPos(float xPos, float yPos);
    RenderData();
    glm::mat4 modelMatrix;


private:
    std::vector<std::shared_ptr<Model>> models;
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