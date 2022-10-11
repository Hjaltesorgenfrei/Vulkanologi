#include "BehPipelines.h"

#include <utility>
#include "Util.h"
#include "Mesh.h"


BehPipeline::BehPipeline(std::shared_ptr<BehDevice>& device, PipelineConfigurationInfo &config) {
    this->device = device;

    if(!config.validate()) {
        std::cerr << "Creating pipeline with invalid configuration.\n";
    }

    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos;
    std::vector<vk::ShaderModule> shaderModules;
    for (const auto& [filepath, stage] : config.shaders) {
        auto vertShaderCode = readFile(filepath);
        shaderModules.push_back(createShaderModule(vertShaderCode));
        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
                .stage = stage,
                .module = shaderModules.back(),
                .pName = "main"
        };
        shaderStageCreateInfos.push_back(shaderStageCreateInfo);
    }

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
            .vertexBindingDescriptionCount = static_cast<uint32_t>(config.bindingDescriptions.size()),
            .pVertexBindingDescriptions = config.bindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(config.attributeDescriptions.size()),
            .pVertexAttributeDescriptions = config.attributeDescriptions.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
            .topology = vk::PrimitiveTopology::eTriangleList,
            .primitiveRestartEnable = VK_FALSE
    };

    vk::Viewport viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(config.extent.width),
            .height = static_cast<float>(config.extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f
    };

    vk::Rect2D scissor {
            .offset = {0, 0},
            .extent = config.extent
    };

    vk::PipelineViewportStateCreateInfo viewportState {
            .viewportCount = 1,
            .pViewports = &viewport,
            .scissorCount = 1,
            .pScissors = &scissor,
    };


    vk::PipelineRasterizationStateCreateInfo rasterizer {
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = config.polygonMode,
            .cullMode = vk::CullModeFlagBits::eBack,
            .frontFace = vk::FrontFace::eCounterClockwise,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp = 0.0f,
            .depthBiasSlopeFactor = 0.0f,
            .lineWidth = 1.0f,
    };


    vk::PipelineMultisampleStateCreateInfo multisampling {
            .rasterizationSamples = device->msaaSamples(),
            .sampleShadingEnable = VK_TRUE,
            .minSampleShading = 0.2f,
            .pSampleMask = nullptr,
            .alphaToCoverageEnable = VK_FALSE,
            .alphaToOneEnable = VK_FALSE,
    };


    vk::PipelineColorBlendAttachmentState colorBlendAttachment {
            .blendEnable = VK_FALSE,
            .colorWriteMask = vk::ColorComponentFlagBits::eR
                              | vk::ColorComponentFlagBits::eG
                              | vk::ColorComponentFlagBits::eB
                              | vk::ColorComponentFlagBits::eA,
    };



    vk::PipelineColorBlendStateCreateInfo colorBlending {
            .logicOpEnable = VK_FALSE,
            .logicOp = vk::LogicOp::eCopy,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil {
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
    };

    vk::GraphicsPipelineCreateInfo pipelineInfo {
            // Reference programmable stages
            .stageCount = static_cast<uint32_t>(shaderStageCreateInfos.size()),

            // Fixed Function stages
            .pStages = shaderStageCreateInfos.data(),
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pDepthStencilState = &depthStencil,
            .pColorBlendState = &colorBlending,

            // A vulkan handle, not a struct pointer
            .layout = config.pipelineLayout,

            .renderPass = config.renderPass,
            .subpass = config.subpass,

            .basePipelineHandle = nullptr,
            .basePipelineIndex = -1,
    };

    auto result = device->device().createGraphicsPipeline(nullptr, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create pipeline!");
    }
    pipeline = result.value;

    for(const auto& shaderModule : shaderModules) {
        device->device().destroyShaderModule(shaderModule);
    }
}

vk::ShaderModule BehPipeline::createShaderModule(const std::vector<char>& code) {
	const vk::ShaderModuleCreateInfo createInfo {
			.codeSize = code.size(),
			.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};

	vk::ShaderModule shaderModule;
	try {
		shaderModule = device->device().createShaderModule(createInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create shader module!") + err.what());
	}

	return shaderModule;
}

void BehPipeline::defaultPipelineConfiguration(PipelineConfigurationInfo& configurationInfo) {
    configurationInfo.bindingDescriptions = Vertex::getBindingDescription();
    configurationInfo.attributeDescriptions = Vertex::getAttributeDescriptions();
    configurationInfo.polygonMode = vk::PolygonMode::eFill;
}

void BehPipeline::bind(vk::CommandBuffer commandBuffer, vk::PipelineBindPoint bindPoint) {
    commandBuffer.bindPipeline(bindPoint, pipeline);
}

BehPipeline::~BehPipeline() {
    device->device().destroyPipeline(pipeline);
}

bool PipelineConfigurationInfo::validate() {
    vk::Flags<vk::ShaderStageFlagBits> currentShaders;
    for (const auto &[_, stage] : shaders) {
        if (currentShaders & stage) {
            std::cerr << "Multiple shaders of the same type!\n";
            return false;
        }
        currentShaders = currentShaders | stage;
    }

    if (!pipelineLayout) {
        std::cerr << "PipelineLayout was null\n";
        return false;
    }

    if (!renderPass) {
        std::cerr << "RenderPass was null\n";
        return false;
    }

    if (!extent.width || !extent.height) {
        std::cerr << "Extent height or width was zero\n";
        return false;
    }

    return true;
}

void PipelineConfigurationInfo::addShader(const std::string& filename, vk::ShaderStageFlagBits shaderStage) {
    shaders.emplace_back(filename, shaderStage);
}
