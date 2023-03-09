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
    bool isComputePipeline = false;
    for (const auto& [filepath, stage] : config.shaders) {
        auto vertShaderCode = readFile(filepath);
        shaderModules.push_back(createShaderModule(vertShaderCode));
        vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
                .stage = stage,
                .module = shaderModules.back(),
                .pName = "main"
        };
        shaderStageCreateInfos.push_back(shaderStageCreateInfo);
        isComputePipeline |= stage == vk::ShaderStageFlagBits::eCompute;
    }

    if (isComputePipeline) {
        vk::ComputePipelineCreateInfo pipelineInfo{
            .stage = shaderStageCreateInfos[0],
            .layout = config.pipelineLayout
        };

        auto result = device->device().createComputePipeline(nullptr, pipelineInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create compute pipeline!");
        }
        pipeline = result.value;
    }
    else {
        createGraphicsPipeline(config, shaderStageCreateInfos);
    }

    for(const auto& shaderModule : shaderModules) {
        device->device().destroyShaderModule(shaderModule);
    }
}

void BehPipeline::createGraphicsPipeline(PipelineConfigurationInfo &config, std::vector<vk::PipelineShaderStageCreateInfo>& shaderStageCreateInfos) {
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
        .vertexBindingDescriptionCount = static_cast<uint32_t>(config.bindingDescriptions.size()),
        .pVertexBindingDescriptions = config.bindingDescriptions.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(config.attributeDescriptions.size()),
        .pVertexAttributeDescriptions = config.attributeDescriptions.data(),
    };

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
        .topology = config.topology,
        .primitiveRestartEnable = VK_FALSE
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
        .lineWidth = config.lineWidth,
    };


    vk::PipelineMultisampleStateCreateInfo multisampling {
        .rasterizationSamples = device->msaaSamples(),
        .sampleShadingEnable = VK_TRUE,
        .minSampleShading = 0.2f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };


    vk::PipelineColorBlendStateCreateInfo colorBlending {
        .logicOpEnable = VK_FALSE,
        .logicOp = vk::LogicOp::eCopy,
        .attachmentCount = 1,
        .pAttachments = &config.colorBlendAttachment,
        .blendConstants = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f}
    };

    vk::PipelineDepthStencilStateCreateInfo depthStencil;

    if (config.topology != vk::PrimitiveTopology::ePointList) {
        depthStencil = {
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable = VK_TRUE,
            .depthCompareOp = vk::CompareOp::eLess,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE
        };
    }


    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eScissor,
        vk::DynamicState::eViewport
    };

    vk::PipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    vk::PipelineViewportStateCreateInfo viewportState {
        .viewportCount = 1,
        .scissorCount = 1,
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
        .pDynamicState = &dynamicStateCreateInfo,

        // A vulkan handle, not a struct pointer
        .layout = config.pipelineLayout,

        .renderPass = config.renderPass,
        .subpass = config.subpass,

        .basePipelineHandle = nullptr,
        .basePipelineIndex = -1,
    };

    auto result = device->device().createGraphicsPipeline(nullptr, pipelineInfo);
    if (result.result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }
    pipeline = result.value;
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

    if (currentShaders & vk::ShaderStageFlagBits::eCompute && shaders.size() > 1) {
        std::cerr << "You can only have one compute shader in a pipeline";
        return false;
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
