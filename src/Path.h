#pragma once

#ifndef PATH_H
#define PATH_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>

struct Point {
    glm::vec3 position;
    glm::vec3 color;
    glm::vec3 up;

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
                .offset = offsetof(Point, color)
            },
            vk::VertexInputAttributeDescription{
                .location = 2,
                .binding = 0,
                .format = vk::Format::eR32G32B32Sfloat,
                .offset = offsetof(Point, up)
            },
        };
    }
};

class Path {
public:
    Path(bool loop = false);

    const bool loop;

    void addPoint(Point point);
    Point getPoint(int index);
    size_t getNumPoints();
    void setPoint(int index, Point point);
    void removePoint(int index);
    void clear();

    std::vector<uint32_t> const & getIndices() const;
    std::vector<Point> const & getPoints() const;

private:
    std::vector<Point> points;
    std::vector<uint32_t> indices;
    
};

Path linePath(glm::vec3 start, glm::vec3 end, glm::vec3 color);

#endif