#pragma once

#include <glm/glm.hpp>

struct Light {
	glm::vec3 position;
	bool isDirectional;
	glm::vec3 color;
	float intensity;
};