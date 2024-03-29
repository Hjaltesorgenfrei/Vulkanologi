#pragma once
#include <vulkan/vulkan.hpp>

#include "BehVkTypes.hpp"
#include "Material.hpp"
#include "Mesh.hpp"

// TODO: This object should in no way be used as a component.
// It should be created for each frame and contain MeshID, MaterialID, Transform, Color and maybe some other stuff.

class RenderObject {
public:
	std::shared_ptr<Mesh> mesh;
	Material material;
	MeshPushConstants transformMatrix = {glm::mat4(1.0f)};
};
