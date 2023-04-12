#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

enum Axis {
    X,
    Y,
    Z
};

std::vector<char> readFile(const std::string& filename);