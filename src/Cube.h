#pragma once

#include "Mesh.h"
#include <memory>

// Creates a cube with texture facing the negative z-axis, centered at the origin with a side length of 1.
std::shared_ptr<Mesh> createCubeMesh(glm::vec3 position);
