#include "Mesh.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std
{
    template <>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const &vertex) const
        {
            return ((hash<glm::vec3>()(vertex.pos) ^
                     (hash<glm::vec3>()(vertex.color) << 1)) >>
                    1) ^
                   (hash<glm::vec2>()(vertex.texCoord) << 1); // TODO: Add the material id here also
        }
    };
} // namespace std

vk::VertexInputBindingDescription Vertex::getBindingDescription()
{
    vk::VertexInputBindingDescription bindingDescription{
        .binding = 0,
        .stride = sizeof(Vertex),
        .inputRate = vk::VertexInputRate::eVertex};
    return bindingDescription;
}

std::array<vk::VertexInputAttributeDescription, 5> Vertex::getAttributeDescriptions()
{
    std::array<vk::VertexInputAttributeDescription, 5> attributeDescriptions = {
        vk::VertexInputAttributeDescription{
            .location = 0,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, pos)},
        vk::VertexInputAttributeDescription{
            .location = 1,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, color)},
        vk::VertexInputAttributeDescription{
            .location = 2,
            .binding = 0,
            .format = vk::Format::eR32G32B32Sfloat,
            .offset = offsetof(Vertex, normal)},
        vk::VertexInputAttributeDescription{
            .location = 3,
            .binding = 0,
            .format = vk::Format::eR32G32Sfloat,
            .offset = offsetof(Vertex, texCoord)},
        vk::VertexInputAttributeDescription{
            .location = 4,
            .binding = 0,
            .format = vk::Format::eR8Uint,
            .offset = offsetof(Vertex, materialIndex)},
    };
    return attributeDescriptions;
}

Mesh Mesh::LoadFromObj(const char *filename)
{
    Mesh mesh{};
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, "./resources"))
    {
        throw std::runtime_error("RenderData failed to load!\n" + warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};
    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t index = shapes[s].mesh.indices[index_offset + v];

                Vertex vertex{
                    .pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2]},
                    .color = {1.0f, 1.0f, 1.0f},
                    .materialIndex = static_cast<uint8_t>(shapes[s].mesh.material_ids[f]) // Index of material which is loaded later
                };

                if (index.normal_index >= 0)
                {
                    vertex.normal = {
                        attrib.normals[3 * index.normal_index + 0],
                        attrib.normals[3 * index.normal_index + 1],
                        attrib.normals[3 * index.normal_index + 2]};
                }

                if (index.texcoord_index >= 0)
                {
                    vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0], 1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};
                }

                if (uniqueVertices.count(vertex) == 0)
                {
                    uniqueVertices[vertex] = static_cast<uint32_t>(mesh._vertices.size());
                    mesh._vertices.push_back(vertex);
                }

                mesh._indices.push_back(uniqueVertices[vertex]);
            }
            index_offset += fv;
        }
    }

    for (const auto &material : materials)
    {
        mesh._texturePaths.push_back("./resources/" + material.diffuse_texname);
    }

    return mesh;
}