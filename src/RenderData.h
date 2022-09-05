#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "RenderObject.h"

struct UniformBufferObject
{
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projView;
};

class RenderData
{
public:
    RenderData();
    ~RenderData();
	RenderData& operator=(const RenderData&) = delete;
	RenderData(const RenderData&) = delete;
    MeshPushConstants getPushConstants();
    const UniformBufferObject getCameraProject(float width, float height);
    std::vector<RenderObject*> getModels();
    void moveCameraForward(float speed);
    void moveCameraBackward(float speed);
    void moveCameraLeft(float speed);
    void moveCameraRight(float speed);
    void resetCursorPos();
    void newCursorPos(float xPos, float yPos);
    glm::mat4 modelMatrix;


private:
    std::vector<RenderObject*> models;
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