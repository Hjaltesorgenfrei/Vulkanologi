#include "Interpolation.hpp"

glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t)
{
    return glm::mix(a, b, t);
}

glm::vec3 quadtricCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, float t)
{
    return lerp(lerp(a, b, t), lerp(b, c, t), t);
}

glm::vec3 cubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t)
{
    return lerp(quadtricCurve(a, b, c, t), quadtricCurve(b, c, d, t), t);
}

glm::vec3 tangentCubicCurve(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d, float t)
{
    return 3.f * glm::pow(1.f - t, 2.f) * (b - a) +
        6.f * (1.f - t) * t * (c - b) +
        3.f * glm::pow(t, 2.f) * (d - c);
}
