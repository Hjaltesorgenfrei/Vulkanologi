#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "BehCamera.hpp"
#include "RenderObject.hpp"
#include "curves/Curves.hpp"
#include "GlobalUbo.hpp"

struct FrameInfo {
    BehCamera camera;
    std::vector<std::shared_ptr<RenderObject>> objects{};
    std::vector<Path> paths{};
    float deltaTime = 0.16f;
};