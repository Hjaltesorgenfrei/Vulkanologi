#pragma once

#include <glm/glm.hpp>
#include <vector>

#include "BehCamera.hpp"
#include "BehLight.hpp"
#include "curves/Curves.hpp"
#include "RenderObject.hpp"

struct FrameInfo {
	BehCamera camera;
	std::vector<RenderObjectNew> objects{};
	std::vector<Path> paths{};
	// Only support 1 light so far.
	std::vector<Light> lights{};
	float deltaTime = 0.16f;
};