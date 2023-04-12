#pragma once

#ifndef PATH_H
#define PATH_H

#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace vk {
    struct VertexInputBindingDescription;
    struct VertexInputAttributeDescription;
}

struct Point {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions();

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions();
};

struct FrenetFrame
{
    glm::vec3 o; // origin of all vectors, i.e. the on-curve point,
    glm::vec3 t; // tangent vector
    glm::vec3 r; // rotational axis vector
    glm::vec3 n; // normal vector

    glm::mat4 rotation() const;

    glm::vec3 binormal() const;
};

glm::mat4 frenetFrameMatrix(FrenetFrame frame);

class Path {
public:
    std::vector<uint32_t> const & getIndices() const;
    std::vector<Point> const & getPoints() const;
    std::vector<FrenetFrame> const & getFrenetFrames() const;
    virtual void recompute();

protected:
    Path(bool loop = false);

    std::vector<Point> points;
    std::vector<uint32_t> indices;
    std::vector<FrenetFrame> frenetFrames; // Might not be needed in all cases, like the line path
    bool dirty = true; // TODO: Use this to only update the buffers when needed
    float resolution = 0.1f; // Distance between points on the curve
    bool loop;
};

#endif