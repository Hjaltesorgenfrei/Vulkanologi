#pragma once
#include "Mesh.hpp"
#include "Material.hpp"
#include "BehVkTypes.hpp"
#include <vulkan/vulkan.hpp>


class RenderObject {
public:
    std::shared_ptr<Mesh> mesh;
    Material material;
    MeshPushConstants transformMatrix = {glm::mat4(1.0f)};
};
