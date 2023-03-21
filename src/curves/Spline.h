#pragma once

#ifndef _SPLINE_H_
#define _SPLINE_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <utility>
#include "Path.h"

glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t);

glm::vec3 quadtricCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, float t);

glm::vec3 cubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t);

// Function that returns tanget at t
// Tangent is first derivative of the Cubic Bezier Curve
glm::vec3 tangent(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t);

std::vector<std::pair<float, float>> arcLength(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int steps);

std::vector<float> evenTsAlongCubic(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int segments, int resolution);


// fernet frame at t
FrenetFrame frenetFrame(glm::vec3 start, glm::vec3 c1, glm::vec3 c2, glm::vec3 end, float t);

std::vector<FrenetFrame> generateRMFrames(Point a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int segments, int resolution);

std::vector<glm::vec3> evenPointsAlongCubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int segments, int resolution);
Path cubicPathVecs(Point a, glm::vec3 b, glm::vec3 c, Point d, int segments, int resolution, glm::vec3 color);
Path cubicPath(ControlPoint a, ControlPoint b, int segments, int resolution, glm::vec3 color);
glm::mat4 moveAlongCubicPath(ControlPoint a, ControlPoint b, int segments, int resolution, float t);

#endif