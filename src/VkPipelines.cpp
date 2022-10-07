#include "VkPipelines.h"
#include "Util.h"
#include "Mesh.h"

bool VkPipelineBuilder::build(DeletionQueue& deletionQueue, vk::Pipeline& pipeline) {
	std::vector<vk::PipelineShaderStageCreateInfo> shaderStageCreateInfos;
	std::vector<vk::ShaderModule> shaderModules;
	for (const auto& [filepath, stage] : _shaders) {
		auto vertShaderCode = readFile(filepath);
		shaderModules.push_back(createShaderModule(vertShaderCode));
		vk::PipelineShaderStageCreateInfo shaderStageCreateInfo {
				.stage = stage,
				.module = shaderModules.back(),
				.pName = "main"
		};
		shaderStageCreateInfos.push_back(shaderStageCreateInfo);
	}

	auto bindingDescriptions = Vertex::getBindingDescription();
	auto attributeDescriptions = Vertex::getAttributeDescriptions();

	vk::PipelineVertexInputStateCreateInfo vertexInputInfo {
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &bindingDescriptions,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
			.pVertexAttributeDescriptions = attributeDescriptions.data(),
	};

	vk::PipelineInputAssemblyStateCreateInfo inputAssembly {
			.topology = vk::PrimitiveTopology::eTriangleList,
			.primitiveRestartEnable = VK_FALSE
	};

	vk::Viewport viewport {
			.x = 0.0f,
			.y = 0.0f,
			.width = static_cast<float>(extent.width),
			.height = static_cast<float>(extent.height),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
	};

	vk::Rect2D scissor {
			.offset = {0, 0},
			.extent = extent
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
			.polygonMode = _polygonMode,
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
			.layout = pipelineLayout,

			.renderPass = renderPass,
			.subpass = 0,

			.basePipelineHandle = nullptr,
			.basePipelineIndex = -1,
	};

	auto result = device->device().createGraphicsPipeline(nullptr, pipelineInfo);
	if (result.result != vk::Result::eSuccess) {
		std::cerr << "failed to create pipeline!\n";
		return false;
	}
	pipeline = result.value;

	deletionQueue.push_function([pipeline, device = device]() {
		device->device().destroyPipeline(pipeline);
	});

	for(const auto& shaderModule : shaderModules) {
		device->device().destroyShaderModule(shaderModule);
	}

	return true;
}

vk::ShaderModule VkPipelineBuilder::createShaderModule(const std::vector<char>& code) {
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

VkPipelineBuilder &VkPipelineBuilder::polygonMode(vk::PolygonMode polygonMode) {
	this->_polygonMode = polygonMode;
	return *this;
}

VkPipelineBuilder VkPipelineBuilder::begin(VulkanDevice *device, vk::Extent2D extent, vk::PipelineLayout pipelineLayout,
										   vk::RenderPass renderPass) {
	VkPipelineBuilder pipelineBuilder{};
	pipelineBuilder.device = device;
	pipelineBuilder.extent = extent;
	pipelineBuilder.pipelineLayout = pipelineLayout;
	pipelineBuilder.renderPass = renderPass;
	return pipelineBuilder;
}

VkPipelineBuilder &VkPipelineBuilder::shader(const std::string& filepath, vk::ShaderStageFlagBits stage) {
	this->_shaders.emplace_back(filepath, stage);
	return *this;
}
