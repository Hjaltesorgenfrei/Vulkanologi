#pragma once

#ifndef PATH_H
#define PATH_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

struct Point {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    static std::vector<vk::VertexInputBindingDescription> getBindingDescriptions() {
        return {
            vk::VertexInputBindingDescription{
                .binding = 0,
                .stride = sizeof(Point),
                .inputRate = vk::VertexInputRate::eVertex
            }
        };
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription{
                .location = 0,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Point, position)
            },
            vk::VertexInputAttributeDescription{
                .location = 1,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Point, normal)
            },
            vk::VertexInputAttributeDescription{
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Point, color)
            }
        };
    }
};

struct FrenetFrame
{
    glm::vec3 o; // origin of all vectors, i.e. the on-curve point,
    glm::vec3 t; // tangent vector
    glm::vec3 r; // rotational axis vector
    glm::vec3 n; // normal vector
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