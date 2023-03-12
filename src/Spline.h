#pragma once

#ifndef _SPLINE_H_
#define _SPLINE_H_

#include <glm/glm.hpp>
#include <vector>
#include <utility>
#include "Path.h"

inline glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t)
{
    return glm::mix(a, b, t);
}

inline glm::vec3 quadtricCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, float t)
{
    return lerp(lerp(a, b, t), lerp(b, c, t), t);
}

inline glm::vec3 cubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t)
{
    return lerp(quadtricCurve(a, b, c, t), quadtricCurve(b, c, d, t), t);
}

// Function that returns tanget at t
// Tangent is first derivative of the Cubic Bezier Curve
inline glm::vec3 tangent(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t)
{
    return 3.f * glm::pow(1.f - t, 2.f) * (b - a) +
        6.f * (1.f - t) * t * (c - b) +
        3.f * glm::pow(t, 2.f) * (d - c);
}

inline std::vector<std::pair<float, float>> arcLength(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int steps)
{
    std::vector<std::pair<float, float>> lengths;
    float length = 0.f;
    glm::vec3 lastPoint = a;
    for (int i = 1; i <= steps; i++)
    {
        float t = (float)i / (float)steps;
        auto point = cubicCurve(a, b, c, d, t);
        length += glm::length(point - lastPoint);
        lastPoint = point;
        lengths.push_back(std::make_pair(length, t));
    }
    return lengths;
}

inline std::vector<float> evenTsAlongCubic(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int segments, int resolution)
{
    std::vector<float> ts;

    auto lengths = arcLength(a, b, c, d, segments * resolution);
    auto length = lengths.back().first;

    float segmentLength = length / (float)segments;
    float currentLength = 0.f;
    int currentSegment = 0;
    for (int i = 0; i < lengths.size(); i++)
    {
        if (lengths[i].first > currentLength)
        {
            ts.push_back(lengths[i].second);
            currentSegment++;
            currentLength = segmentLength * (float)currentSegment;
        }
    }

    return ts;
}


struct FrenetFrame
{
    glm::vec3 o; // origin of all vectors, i.e. the on-curve point,
    glm::vec3 t; // tangent vector
    glm::vec3 r; // rotational axis vector
    glm::vec3 n; // normal vector
};


// fernet frame at t
inline FrenetFrame frenetFrame(glm::vec3 start, glm::vec3 c1, glm::vec3 c2, glm::vec3 end, float t) {
    auto t1 = tangent(start, c1, c2, end, t);
    auto a = glm::normalize(t1);
    auto b = glm::normalize(a + glm::cross(a, glm::vec3(0.f, 1.f, 0.f)));
    auto r = glm::normalize(glm::cross(b, a));
    auto normal = glm::normalize(glm::cross(r, a));
    return FrenetFrame{ cubicCurve(start, c1, c2, end, t), a, r, normal };
}

// C++ translation of https://pomax.github.io/bezierinfo/#pointvectors3d

inline std::vector<FrenetFrame> generateRMFrames(Point a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int segments, int resolution)
{
    std::vector<FrenetFrame> frames;

    auto ts = evenTsAlongCubic(a.position, b, c, d, segments, resolution);

    auto x0 = FrenetFrame {
        .o = a.position,
        .t = tangent(a.position, b, c, d, ts[0]),
        .n = a.normal
    };
    
    // Get right from normal and tangent
    x0.r = glm::normalize(glm::cross(x0.n, x0.t));

    frames.push_back(x0);

    for (int i = 1; i < ts.size(); i++)
    {
        x0 = frames.back();

        auto t1 = ts[i];
        auto x1 = FrenetFrame{ .o = cubicCurve(a.position, b, c, d, t1), .t = tangent(a.position, b, c, d, t1) };

        auto v1 = x1.o - x0.o;
        auto c1 = glm::dot(v1, v1);
        auto niL = x0.n - v1 * 2.f / c1 * glm::dot(v1, x0.n);
        auto tiL = x0.t - v1 * 2.f / c1 * glm::dot(v1, x0.t);

        auto v2 = x1.t - tiL;
        auto c2 = glm::dot(v2, v2);

        x1.n = niL - v2 * 2.f / c2 * glm::dot(v2, niL);
        x1.r = glm::cross(x1.n, x1.t);
        frames.push_back(x1);
    }

    return frames;
}

inline std::vector<glm::vec3> evenPointsAlongCubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int segments, int resolution)
{
    std::vector<glm::vec3> points;
    
    for (auto t : evenTsAlongCubic(a, b, c, d, segments, resolution))
    {
        points.push_back(cubicCurve(a, b, c, d, t));
    }

    return points;
}

Path cubicPath(Point a, glm::vec3 b, glm::vec3 c, glm::vec3 d, int segments, int resolution, glm::vec3 color)
{
    Path path;
    auto points = evenPointsAlongCubicCurve(a.position, b, c, d, segments, resolution);
    auto frames = generateRMFrames(a, b, c, d, segments, resolution);

    for (int i = 0; i < points.size(); i++)
    {
        auto point = points[i];
        auto frame = frames[i];
        auto normal = frame.n;
        Point p{ 
            .position = point, 
            .normal = glm::normalize(normal), 
            .color = color
        };
        path.addPoint(p);
    }

    return path;  
}

#endif