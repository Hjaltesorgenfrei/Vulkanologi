#include "BehDevice.hpp"
#include "Deletionqueue.hpp"

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
    vk::PrimitiveTopology topology = vk::PrimitiveTopology::eTriangleList;
    vk::CullModeFlags cullMode = vk::CullModeFlagBits::eBack;
    bool useDepth = true;
    float lineWidth = 1.0f;

    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
        .blendEnable = VK_FALSE,
        .colorWriteMask = vk::ColorComponentFlagBits::eR
                          | vk::ColorComponentFlagBits::eG
                          | vk::ColorComponentFlagBits::eB
                          | vk::ColorComponentFlagBits::eA,
    };

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

    void createGraphicsPipeline(PipelineConfigurationInfo &config,
                           std::vector<vk::PipelineShaderStageCreateInfo>& shaderStageCreateInfos);
};