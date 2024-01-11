#pragma once
#include <vulkan/vulkan.hpp>

#include "BehVkTypes.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

class RenderObject {
public:
	std::shared_ptr<Mesh> mesh;
	Material material;
	MeshPushConstants transformMatrix = {glm::mat4(1.0f)};
};
