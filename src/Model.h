#pragma once
#include "Mesh.h"
#include "Material.h"
#include "VkTypes.h"
#include <vulkan/vulkan.hpp>


class Model{
public: 
    Mesh mesh;
    Material material;
    MeshPushConstants transformMatrix;
    Model(Mesh mesh, Material material);
    void Draw(vk::CommandBuffer commandBuffer);
};
