#include "BehDevice.h"
#include "Deletionqueue.h"

#pragma once

struct PipelineConfigurationInfo {
    std::vector<vk::VertexInputBindingDescription> bindingDescriptions{};
    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};
    vk::PolygonMode polygonMode;

    // The following four values have to filled for the config to be valid.
    std::vector<std::pair<std::string, vk::ShaderStageFlagBits>> shaders;
    vk::Extent2D extent{};
    vk::PipelineLayout pipelineLayout = nullptr;
    vk::RenderPass renderPass = nullptr;

    uint32_t subpass = 0;

    void addShader(const std::string& filename, vk::ShaderStageFlagBits shaderStage);
    bool validate();
};

class BehPipeline {
public:
    BehPipeline(std::shared_ptr<BehDevice>& device, PipelineConfigurationInfo& config);
    ~BehPipeline();

    BehPipeline(const BehPipeline&) = delete;
    BehPipeline& operator=(const BehPipeline&) = delete;

    void bind(vk::CommandBuffer commandBuffer, vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics);

    static void defaultPipelineConfiguration(PipelineConfigurationInfo& configurationInfo);

private:
    std::shared_ptr<BehDevice> device;
    vk::Pipeline pipeline;

    vk::ShaderModule createShaderModule(const std::vector<char> &code);
};