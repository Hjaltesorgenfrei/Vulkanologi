#pragma once
#include "Mesh.h"
#include "Material.h"
#include "BehVkTypes.h"
#include <vulkan/vulkan.hpp>


class RenderObject {
public: 
    Mesh mesh;
    Material material;
    MeshPushConstants transformMatrix;
    RenderObject(Mesh mesh, Material material);
    void Draw(vk::CommandBuffer commandBuffer);
};
