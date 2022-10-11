#include "RenderObject.h"

RenderObject::RenderObject(std::shared_ptr<Mesh> mesh, Material material) {
    this->material = material;
    this->mesh = mesh;
    transformMatrix = {glm::mat4(1.0f)};
}

void RenderObject::Draw(vk::CommandBuffer commandBuffer) {
    vk::Buffer vertexBuffers[] = {mesh->_vertexBuffer._buffer};
    vk::DeviceSize offsets[] = {0};
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

    commandBuffer.bindIndexBuffer(mesh->_indexBuffer._buffer, 0, vk::IndexType::eUint32);

    commandBuffer.drawIndexed(static_cast<uint32_t>(mesh->_indices.size()), 1, 0, 0, 0);
}