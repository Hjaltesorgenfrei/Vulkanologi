#include "Model.h"
#include "RenderData.h"

Model::Model(Mesh mesh, Material material) {
    this->material = material;
    this->mesh = mesh;
    transformMatrix.model = glm::mat4(1.0f);
}

void Model::Draw(vk::CommandBuffer commandBuffer) {
    vk::Buffer vertexBuffers[] = { mesh._vertexBuffer._buffer };
    vk::DeviceSize offsets[] = { 0 };
    commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);

    commandBuffer.bindIndexBuffer(mesh._indexBuffer._buffer, 0, vk::IndexType::eUint32);

    commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, material.pipelineLayout, 0, 1, &material.textureSet, 0, nullptr);

    commandBuffer.pushConstants(material.pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(MeshPushConstants), &transformMatrix);
    
    commandBuffer.drawIndexed(static_cast<uint32_t>(mesh._indices.size()), 1, 0, 0, 0);
}