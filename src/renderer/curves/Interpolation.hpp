#pragma once

#ifndef INTERPOLATION_H
#define INTERPOLATION_H

#include <glm/glm.hpp>

glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t);

glm::vec3 quadtricCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, float t);

glm::vec3 cubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t);

// Function that returns tanget at t
// Tangent is first derivative of the Cubic Bezier Curve
glm::vec3 tangentCubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t);

#endif