#pragma once

#include <glm/glm.hpp>

struct GlobalUbo {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 projView;
    glm::vec4 ambientLightColor{1.f, 1.f, 1.f, 0.4f};  // w is intensity
    glm::vec4 lightPosition{3.f, -3.0f, 7.5f, 0.0f}; // w is ignored
    glm::vec4 lightColor{1.f, 1.f, 1.f, 30.f};  // w is light intensity
    float deltaTime;
};