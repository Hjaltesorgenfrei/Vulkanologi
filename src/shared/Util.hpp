#pragma once
#include <string>
#include <vector>

enum Axis { X, Y, Z };

std::vector<char> readFile(const std::string& filename);