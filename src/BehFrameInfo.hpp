#pragma once

#include <glm/glm.hpp>
#include "BehCamera.hpp"
#include "RenderObject.hpp"
#include "curves/Curves.hpp"

struct GlobalUbo {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projView;
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 1.f};  // w is intensity
    glm::vec4 lightPosition{0.f, 1.0f, 0.0f, 0.0f}; // w is ignored
    glm::vec4 lightColor{1.f, 1.f, 1.f, 5.f};  // w is light intensity
    float deltaTime;
};

struct FrameInfo {
    BehCamera camera;
    std::vector<std::shared_ptr<RenderObject>> objects{};
    std::vector<Path> paths{};
    float deltaTime = 0.16f;
};