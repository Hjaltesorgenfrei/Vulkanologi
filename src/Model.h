#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Vertice.h"

struct UniformBufferObject
{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
};

class Model
{
public:
    const std::vector<Vertex> getVertices();
    const std::vector<uint16_t> getIndices();
    const UniformBufferObject getCameraProject(float width, float height);
    Model();

private:
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    UniformBufferObject ubo;
};