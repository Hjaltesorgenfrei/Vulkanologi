#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "RenderObject.h"

struct UniformBufferObject {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projView;
};

class RenderData {
   public:
    RenderData();
    ~RenderData();
    RenderData& operator=(const RenderData&) = delete;
    RenderData(const RenderData&) = delete;
    const UniformBufferObject getCameraProject(float width, float height);
    std::vector<RenderObject*> getModels();
    void moveCameraForward(float speed);
    void moveCameraBackward(float speed);
    void moveCameraLeft(float speed);
    void moveCameraRight(float speed);
    void resetCursorPos();
    void newCursorPos(float xPos, float yPos);

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