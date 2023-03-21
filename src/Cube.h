#pragma once

#include "Mesh.h"
#include <memory>

// Creates a cube with texture facing the negative z-axis and side length 1.
std::shared_ptr<Mesh> createCubeMesh();
