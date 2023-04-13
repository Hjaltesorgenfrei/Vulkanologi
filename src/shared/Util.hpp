#pragma once
#include <vector>
#include <string>

enum Axis {
    X,
    Y,
    Z
};

std::vector<char> readFile(const std::string& filename);