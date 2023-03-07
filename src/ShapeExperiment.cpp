#include "ShapeExperiment.h"

std::shared_ptr<Mesh> createCube(glm::vec3 position)
{
    auto mesh = std::make_shared<Mesh>(); 

    std::vector<glm::vec3> pos = {
        { -0.5f, -0.5f, -0.5f },
        {  0.5f, -0.5f, -0.5f },
        {  0.5f,  0.5f, -0.5f },
        { -0.5f,  0.5f, -0.5f },
        { -0.5f, -0.5f,  0.5f },
        {  0.5f, -0.5f,  0.5f },
        {  0.5f,  0.5f,  0.5f },
        { -0.5f,  0.5f,  0.5f }
    };

    std::vector<glm::vec3> normals = {
        {  0.0f,  0.0f, -1.0f },
        {  0.0f,  0.0f,  1.0f },
        {  0.0f, -1.0f,  0.0f },
        {  0.0f,  1.0f,  0.0f },
        { -1.0f,  0.0f,  0.0f },
        {  1.0f,  0.0f,  0.0f }
    };

    std::vector<glm::vec2> texCoords = {
        { 0.0f, 0.0f },
        { 1.0f, 0.0f },
        { 1.0f, 1.0f },
        { 0.0f, 1.0f }
    };

    glm::vec3 color = { 1.0f, 1.0f, 1.0f };

    std::vector<Vertex> vertices = {
        { pos[2], color, normals[0], texCoords[0], 0 },
        { pos[1], color, normals[0], texCoords[3], 0 },
        { pos[3], color, normals[0], texCoords[1], 0 },
        { pos[0], color, normals[0], texCoords[2], 0 },
    };

    std::vector<uint32_t> indices = {
        0, 1, 2,
        1, 3, 2
    };

    mesh->_vertices = vertices;
    mesh->_indices = indices;

    return mesh;
}