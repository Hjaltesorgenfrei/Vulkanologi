#include "RenderObject.hpp"

RenderObject::RenderObject(std::shared_ptr<Mesh> mesh, Material material) {
    this->material = material;
    this->mesh = mesh;
    transformMatrix = {glm::mat4(1.0f)};
}