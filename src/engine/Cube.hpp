#pragma once

#include "Mesh.hpp"
#include <memory>

// Creates a cube with texture facing the negative z-axis and side length 1.
std::shared_ptr<Mesh> createCubeMesh();
