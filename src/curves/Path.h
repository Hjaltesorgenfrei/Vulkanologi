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

class ControlPoint {

public:
    Point point() const {
        Point newPoint;
        newPoint.position = glm::vec3(transform * glm::vec4(_point.position, 1.0f));
        newPoint.normal = glm::vec3(transform * glm::vec4(_point.normal, 0.0f));
        newPoint.color = _point.color;
        return newPoint;
    }

    void setPoint(Point point) {
        _point = point;
    }

    glm::vec3 forward() const {
        return glm::vec3(transform * glm::vec4(_forward, 0.0f));
    }

    glm::vec3 forwardWorld() const {
        return point().position + forward();
    }

    glm::vec3 backwardWorld() const {
        return point().position - forward();
    }

    void setForward(glm::vec3 forward) {
        _forward = forward;
    }

    glm::mat4 transform = glm::mat4(1.0f);

private:
    Point _point = { glm::vec3(0.0f), glm::vec3(0, 1, 0), glm::vec3(1.0f) };
    glm::vec3 _forward = {1.0f, 0.f, 0.f}; // Is mirrored if the points is not a end point

};

struct FrenetFrame
{
    glm::vec3 o; // origin of all vectors, i.e. the on-curve point,
    glm::vec3 t; // tangent vector
    glm::vec3 r; // rotational axis vector
    glm::vec3 n; // normal vector
};

class Path {
public:
    Path(bool loop = false);

    bool loop;

    void addPoint(Point point);
    Point getPoint(int index);
    size_t getNumPoints();
    void setPoint(int index, Point point);
    void removePoint(int index);
    void clear();

    std::vector<uint32_t> const & getIndices() const;
    std::vector<Point> const & getPoints() const;
    std::vector<ControlPoint> const & getControlPoints() const;
    virtual void generateFrenetFrames();

private:
    std::vector<Point> points;
    std::vector<uint32_t> indices;
    std::vector<ControlPoint> controlPoints;
    std::vector<FrenetFrame> frenetFrames; // Might not be needed in all cases, like the line path
    bool dirty = true; // TODO: Use this to only update the buffers when needed
    float resolution = 0.1f; // Distance between points on the curve
};

#endif