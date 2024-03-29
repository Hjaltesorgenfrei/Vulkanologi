#pragma once

#include <memory>

#include "Mesh.hpp"

// Creates a cube with texture facing the negative z-axis and side length 1.
std::shared_ptr<Mesh> createCubeMesh(std::string texturePath = "./resources/white.png");
