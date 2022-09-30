#include "Renderer.h"
#include "Mesh.h"

#include <chrono>
#include <algorithm>
#include <fstream>
#include <set>
#include <map>
#include <stdexcept>
#include <iostream>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"


std::vector<char> readFile(const std::string& filename) {
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open shader file!");
	}

	const size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}

Renderer::Renderer(std::shared_ptr<WindowWrapper> window, std::shared_ptr<VulkanDevice> device, AssetManager &assetManager,
                   std::shared_ptr<RenderData> &renderData)
        : window(window), device{device}, assetManager(assetManager) {
	this->renderData = renderData;
	try {
        descriptorAllocator.init(device->device());
        descriptorLayoutCache.init(device->device());
        mainDeletionQueue.push_function([&](){
            descriptorLayoutCache.cleanup();
            descriptorAllocator.cleanup();
        });
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		uploadMeshes();
		createGraphicsPipelineLayout();
		createGraphicsPipeline();
		createCommandPool();
		initImgui();
		createColorResources();
		createDepthResources();
		createFramebuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
	}
	catch (const std::exception& e) {
		std::cerr << "Renderer failed to initialize!" << std::endl;
		std::cerr << "Error: " << e.what() << std::endl;
		throw e;
	}
}

Renderer::~Renderer() {
	cleanup();
}

void Renderer::frameBufferResized() {
	frameBufferResizePending = true;
}

Material Renderer::createMaterial(std::vector<std::string>& texturePaths) {
	std::vector<std::shared_ptr<UploadedTexture>> textures;
	std::map<std::string, std::shared_ptr<UploadedTexture>> uploadedTextures;
	for (const auto& filename : texturePaths) {
		textures.push_back(assetManager.getTexture(filename));
	}


    std::vector<vk::DescriptorImageInfo> imageInfos;
    for (const auto& texture : textures) {
        vk::DescriptorImageInfo imageInfo {
                .sampler = texture->textureSampler,
                .imageView = texture->textureImageView,
                .imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
        };
        imageInfos.push_back(imageInfo);
    }

    Material material{};
    auto textureResult = DescriptorBuilder::begin(&descriptorLayoutCache, &descriptorAllocator)
            .bindImages(0, imageInfos, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
            .build(material.textureSet, textureDescriptorSetLayout);

    if(!textureResult) {
        throw std::runtime_error("Failed to create Material");
    }

	return material;
}

void Renderer::createSwapChain() {
	const SwapChainSupportDetails swapChainSupport = device->swapChainSupport();

	const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	const vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

    swapChainImageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && swapChainImageCount > swapChainSupport.capabilities.maxImageCount) {
        swapChainImageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo {
		.surface = device->surface(),
		.minImageCount = swapChainImageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment
	};


	QueueFamilyIndices indices = device->queueFamilies();
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		createInfo.queueFamilyIndexCount = 0;
		createInfo.pQueueFamilyIndices = nullptr;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = nullptr;

	try {
		swapChain = device->device().createSwapchainKHR(createInfo);
		swapChainDeletionQueue.push_function([&]() {
			device->device().destroySwapchainKHR(swapChain);
		});
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to create swap chain!") + err.what());
	}

	swapChainImages = device->device().getSwapchainImagesKHR(swapChain);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Renderer::recreateSwapchain() {
	int width = 0, height = 0;
	auto glfwWindow = window->getGLFWwindow();
	glfwGetFramebufferSize(glfwWindow, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(glfwWindow, &width, &height);
		glfwWaitEvents();
	}
	
	device->device().waitIdle(); // Has to wait for rendering to finish before creating new swapchain. Better solutions exists.
	
	swapChainDeletionQueue.flush();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createColorResources();
	createDepthResources();
	createFramebuffers();

	imagesInFlight.resize(swapChainImages.size(), nullptr);
	std::cout << "Recreated swapchain" << std::endl;
}

vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace ==
			vk::ColorSpaceKHR::eSrgbNonlinear) {
			return availableFormat;
		}
	}

	// Just return the first one if we cant find the specified one, it should be okay.
	return availableFormats[0];
}

vk::PresentModeKHR Renderer::chooseSwapPresentMode(const std::vector<enum vk::PresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
			// Vertical sync with less latency, by discarding old frames.
			return availablePresentMode;
		}
	}

	return vk::PresentModeKHR::eFifo; // Standard Vsync, Guaranteed by Vulkan to be available.
}

vk::Extent2D Renderer::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width != UINT32_MAX) {
		return capabilities.currentExtent;
	}
	else {
		const auto [width, height] = window->getFramebufferSize();
		
		vk::Extent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

void Renderer::initImgui() {
	//1: create descriptor pool for IMGUI
	// the size of the pool is very oversize, but it's copied from imgui demo itself.
	vk::DescriptorPoolSize pool_sizes[] =
	{
		{ vk::DescriptorType::eSampler, 1000 },
		{ vk::DescriptorType::eCombinedImageSampler, 1000 },
		{ vk::DescriptorType::eSampledImage, 1000 },
		{ vk::DescriptorType::eStorageImage, 1000 },
		{ vk::DescriptorType::eUniformTexelBuffer, 1000 },
		{ vk::DescriptorType::eStorageTexelBuffer, 1000 },
		{ vk::DescriptorType::eUniformBuffer, 1000 },
		{ vk::DescriptorType::eStorageBuffer, 1000 },
		{ vk::DescriptorType::eUniformBufferDynamic, 1000 },
		{ vk::DescriptorType::eStorageBufferDynamic, 1000 },
		{ vk::DescriptorType::eInputAttachment, 1000 }
	};

	vk::DescriptorPoolCreateInfo pool_info {
		.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
		.maxSets = 1000,
		.poolSizeCount = static_cast<uint32_t>(std::size(pool_sizes)),
		.pPoolSizes = pool_sizes
	};

	imguiPool = device->device().createDescriptorPool(pool_info);

	//this initializes the core structures of imgui
	ImGui::CreateContext();

	//this initializes imgui for Glfw
	ImGui_ImplGlfw_InitForVulkan(window->getGLFWwindow(), true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = device->instance();
	init_info.PhysicalDevice = device->physicalDevice();
	init_info.Device = device->device();
	init_info.Queue = device->graphicsQueue();
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = device->swapChainSupport().capabilities.minImageCount;
	init_info.ImageCount = swapChainImageCount;
	init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(device->msaaSamples());

	ImGui_ImplVulkan_Init(&init_info, renderPass);

	//execute a gpu command to upload imgui font textures
    device->immediateSubmit([&](auto cmd){
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	mainDeletionQueue.push_function([&]() {
		device->device().destroyDescriptorPool(imguiPool);
		ImGui_ImplVulkan_Shutdown();
	});
}

void Renderer::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(device->device(), swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
		swapChainDeletionQueue.push_function([&, i]() {
			device->device().destroyImageView(swapChainImageViews[i]);
		});
	}
}

void Renderer::createRenderPass() {
	vk::AttachmentDescription colorAttachment {
		.format = swapChainImageFormat,
		.samples = device->msaaSamples(),
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::eColorAttachmentOptimal
	};

	vk::AttachmentReference colorAttachmentRef {
		.attachment = 0,
		.layout = vk::ImageLayout::eColorAttachmentOptimal
	};

	vk::AttachmentDescription depthAttachment {
		.format = findDepthFormat(),
		.samples = device->msaaSamples(),
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
	};

	vk::AttachmentReference depthAttachmentRef {
		.attachment = 1,
		.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
	};

	vk::AttachmentDescription colorAttachmentResolve { 
		.format = swapChainImageFormat,
		.samples = vk::SampleCountFlagBits::e1,
		.loadOp = vk::AttachmentLoadOp::eDontCare,
		.storeOp = vk::AttachmentStoreOp::eDontCare,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::ePresentSrcKHR
	};

	vk::AttachmentReference colorAttachmentResolveRef{
		.attachment = 2,
		.layout = vk::ImageLayout::eColorAttachmentOptimal
	};

	vk::SubpassDescription subpass {
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = &colorAttachmentResolveRef,
		.pDepthStencilAttachment = &depthAttachmentRef
	};

	vk::SubpassDependency dependency {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
		.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
        .srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
        .dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead
	};

	std::array<vk::AttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

	const vk::RenderPassCreateInfo renderPassInfo {
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	try {
		renderPass = device->device().createRenderPass(renderPassInfo);
		swapChainDeletionQueue.push_function([&]() {
            device->device().destroyRenderPass(renderPass);
		});
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to create render pass!") + err.what());
	}

}

void Renderer::createDescriptorSetLayout() {
	vk::DescriptorSetLayoutBinding uboLayoutBinding {
		.binding = 0,
		.descriptorType = vk::DescriptorType::eUniformBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
		.pImmutableSamplers = nullptr
	};

	std::array<vk::DescriptorSetLayoutBinding, 1> bindings = {uboLayoutBinding};

	vk::DescriptorSetLayoutCreateInfo layoutInfo {
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	if(device->device().createDescriptorSetLayout(&layoutInfo, nullptr, &uboDescriptorSetLayout) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
	mainDeletionQueue.push_function([&]() {
		device->device().destroyDescriptorSetLayout(uboDescriptorSetLayout);
	});
}

void Renderer::createGraphicsPipelineLayout() {
	 vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(MeshPushConstants)
    };

	std::array<vk::DescriptorSetLayout, 2> descriptorSetLayouts = {uboDescriptorSetLayout, textureDescriptorSetLayout}; 

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
		.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size()),
		.pSetLayouts = descriptorSetLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
	};

	try {
		pipelineLayout = device->device().createPipelineLayout(pipelineLayoutInfo);
		mainDeletionQueue.push_function([&]() {
			device->device().destroyPipelineLayout(pipelineLayout);
		});
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to create pipeline layout!") + err.what());
	}
}

void Renderer::createGraphicsPipeline() {
	auto vertShaderCode = readFile("shaders/shader.vert.spv");
	auto fragShaderCode = readFile("shaders/shader.frag.spv");

	auto vertShaderModule = createShaderModule(vertShaderCode);
	auto fragShaderModule = createShaderModule(fragShaderCode);

	vk::PipelineShaderStageCreateInfo vertShaderStageInfo {
		.stage = vk::ShaderStageFlagBits::eVertex,
		.module = vertShaderModule,
		.pName = "main"
	};
	vk::PipelineShaderStageCreateInfo fragShaderStageInfo {
		.stage = vk::ShaderStageFlagBits::eFragment,
		.module = fragShaderModule,
		.pName = "main"
	};
	vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

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
		.width = static_cast<float>(swapChainExtent.width),
		.height = static_cast<float>(swapChainExtent.height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	vk::Rect2D scissor {
		.offset = {0, 0},
		.extent = swapChainExtent
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
		.polygonMode = vk::PolygonMode::eFill,
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
		.stageCount = 2,

		// Fixed Function stages
		.pStages = shaderStages,
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
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	graphicsPipeline = result.value;
	swapChainDeletionQueue.push_function([&]() {
		device->device().destroyPipeline(graphicsPipeline);
	});

	device->device().destroyShaderModule(vertShaderModule);
	device->device().destroyShaderModule(fragShaderModule);
}

vk::ShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
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

void Renderer::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		std::array<vk::ImageView, 3> attachments = {
			colorImageView,
			depthImageView,
			swapChainImageViews[i]
		};

		vk::FramebufferCreateInfo framebufferInfo {
			.renderPass = renderPass,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1,
		};

		try {
			swapChainFramebuffers[i] = device->device().createFramebuffer(framebufferInfo);
			swapChainDeletionQueue.push_function([&, i]() {
				device->device().destroyFramebuffer(swapChainFramebuffers[i]);
			});
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("Failed to create framebuffer") + err.what());
		}
	}
}

void Renderer::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = device->queueFamilies();
	const vk::CommandPoolCreateInfo poolInfo {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
	};

    const vk::CommandPoolCreateInfo transferPoolInfo {
            .queueFamilyIndex = queueFamilyIndices.transferFamily.value(),
    };

	try {
		commandPool = device->device().createCommandPool(poolInfo);
        transferCommandPool = device->device().createCommandPool(transferPoolInfo);
		mainDeletionQueue.push_function([&]() {
			device->device().destroyCommandPool(commandPool);
			device->device().destroyCommandPool(transferCommandPool);
        });
    }
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create graphics command pools!") + err.what());
	}
}

void Renderer::createColorResources() {
	auto colorFormat = swapChainImageFormat;

	colorImage = assetManager.createImage(
		swapChainExtent.width, 
		swapChainExtent.height, 
		1, 
		device->msaaSamples(),
		colorFormat, 
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment
	);
	colorImageView = createImageView(device->device(), colorImage._image, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
	swapChainDeletionQueue.push_function([&]() {
		device->device().destroyImageView(colorImageView);
		vmaDestroyImage(device->allocator(), colorImage._image, colorImage._allocation);
	});
}

void Renderer::createDepthResources() {
	auto depthFormat = findDepthFormat();
	depthImage = assetManager.createImage(
		swapChainExtent.width, 
		swapChainExtent.height, 
		1,
        device->msaaSamples(),
		depthFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment
	);
	depthImageView = createImageView(device->device(), depthImage._image, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

	transitionImageLayout(depthImage._image, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
	swapChainDeletionQueue.push_function([&]() {
		device->device().destroyImageView(depthImageView);
		vmaDestroyImage(device->allocator(), depthImage._image, depthImage._allocation);
	});
}

vk::Format Renderer::findDepthFormat() {
	return findSupportedFormat(
		{vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
		vk::ImageTiling::eOptimal,
		vk::FormatFeatureFlagBits::eDepthStencilAttachment
	);
}

bool Renderer::hasStencilComponent(vk::Format format) {
	return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

vk::Format Renderer::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
	for (auto& format : candidates) {
		auto props = device->physicalDevice().getFormatProperties(format);

		if(tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
			return format;
		} else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("Failed to find supported format!");
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
	vk::PhysicalDeviceMemoryProperties memoryProperties;
	device->physicalDevice().getMemoryProperties(&memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void Renderer::uploadMeshes() {
	for (auto model : renderData->getModels()) {
        model->mesh._vertexBuffer = uploadBuffer(model->mesh._vertices, VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        model->mesh._indexBuffer = uploadBuffer(model->mesh._indices, VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		mainDeletionQueue.push_function([&, model]() {
			vmaDestroyBuffer(device->allocator(), model->mesh._vertexBuffer._buffer, model->mesh._vertexBuffer._allocation);
			vmaDestroyBuffer(device->allocator(), model->mesh._indexBuffer._buffer, model->mesh._indexBuffer._allocation);
        });
		model->material = createMaterial(model->mesh._texturePaths);
	}
}

template<typename T>
AllocatedBuffer Renderer::uploadBuffer(std::vector<T>& meshData, VkBufferUsageFlags usage) {
    VkBufferCreateInfo stagingCreate {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = meshData.size() * sizeof(T),
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };

    VmaAllocationCreateInfo stagingAlloc {
			.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO
    };

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    if (vmaCreateBuffer(device->allocator(), &stagingCreate, &stagingAlloc, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to uploadBuffer mesh vertices!");
    }

    void *data;
    vmaMapMemory(device->allocator(), stagingAllocation, &data);
    {
        memcpy(data, meshData.data(), meshData.size() * sizeof(T));
    }
    vmaUnmapMemory(device->allocator(), stagingAllocation);

    VkBufferCreateInfo bufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = meshData.size() * sizeof(T),
            .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    };

    VmaAllocationCreateInfo vmaAllocInfo {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    VkBuffer buffer;

    AllocatedBuffer allocatedBuffer{};

    if (vmaCreateBuffer(device->allocator(), &bufferCreateInfo, &vmaAllocInfo, &buffer, &allocatedBuffer._allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to upload buffer!");
    }
    allocatedBuffer._buffer = buffer;

    copyBuffer(stagingBuffer, buffer, meshData.size() * sizeof(T));

    vmaDestroyBuffer(device->allocator(), stagingBuffer, stagingAllocation);
    return allocatedBuffer;
}

void Renderer::createUniformBuffers() {
	uniformBuffers.resize(swapChainImages.size());
    VkBufferCreateInfo create {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = sizeof(UniformBufferObject),
            .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    };

    VmaAllocationCreateInfo allocCreate {
            .usage = VMA_MEMORY_USAGE_CPU_TO_GPU
    };

	for(size_t i = 0; i < swapChainImages.size(); i++) {
        VkBuffer buffer;
        VmaAllocation allocation{};
        if (vmaCreateBuffer(device->allocator(), &create, &allocCreate, &buffer, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to upload buffer!");
        }
        uniformBuffers[i]._buffer = buffer;
		uniformBuffers[i]._allocation = allocation;
		mainDeletionQueue.push_function([&, i]() {
			vmaDestroyBuffer(device->allocator(), uniformBuffers[i]._buffer, uniformBuffers[i]._allocation);
		});
	}
}

void Renderer::createDescriptorPool() {
	std::array<vk::DescriptorPoolSize, 2> poolSizes	{
		vk::DescriptorPoolSize {
			.type = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = static_cast<uint32_t>(swapChainImages.size())
		},
		vk::DescriptorPoolSize {
			.type = vk::DescriptorType::eCombinedImageSampler,
			.descriptorCount = static_cast<uint32_t>(swapChainImages.size())
		}
	};

	vk::DescriptorPoolCreateInfo poolInfo {
		.maxSets = static_cast<uint32_t>(swapChainImages.size()) + 100, // This is an extremely bad way to do this. TODO Fix it
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	if(device->device().createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create descriptor pool");
	}
	mainDeletionQueue.push_function([&]() {
		device->device().destroyDescriptorPool(descriptorPool, nullptr);
	});
}

void Renderer::createDescriptorSets() {
	std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), uboDescriptorSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo {
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size()),
		.pSetLayouts = layouts.data()
	};

	descriptorSets.resize(swapChainImages.size());
	if(device->device().allocateDescriptorSets(&allocInfo, descriptorSets.data()) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create Descriptor sets!");
	}


	for(size_t i = 0; i < swapChainImages.size(); i++) {
		vk::DescriptorBufferInfo bufferInfo {
			.buffer = uniformBuffers[i]._buffer,
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		std::array<vk::WriteDescriptorSet, 1> descriptorWrites {
			vk::WriteDescriptorSet {
				.dstSet = descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo = &bufferInfo,
			}
		};

		device->device().updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	device->immediateSubmit([&](vk::CommandBuffer cmd){
		vk::BufferCopy copyRegion{
			.size = size
		};
		cmd.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
	});
}

void Renderer::createCommandBuffers() {
	commandBuffers.resize(swapChainFramebuffers.size());

	vk::CommandBufferAllocateInfo allocateInfo {
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
	};

	try {
		commandBuffers = device->device().allocateCommandBuffers(allocateInfo);
		mainDeletionQueue.push_function([&]() {
			device->device().freeCommandBuffers(commandPool, commandBuffers);
		});
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to allocate command buffers") + err.what());
	}
}

void Renderer::recordCommandBuffer(int index) {
	vk::CommandBufferBeginInfo beginInfo {};

		try {
			commandBuffers[index].begin(beginInfo);
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("failed to begin recording command buffer!") + err.what());
		}

		std::array<vk::ClearValue, 2> clearValues{};
		clearValues[0].color = {std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f }};
		clearValues[1].depthStencil = vk::ClearDepthStencilValue {1.0f, 0};

		vk::RenderPassBeginInfo renderPassInfo {
			.renderPass = renderPass,
			.framebuffer = swapChainFramebuffers[index],
			.renderArea = {
				.offset = {0, 0},
				.extent = swapChainExtent,
			},
			.clearValueCount = static_cast<uint32_t>(clearValues.size()),
			.pClearValues = clearValues.data()
		};

		commandBuffers[index].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		{
			//Add commands to buffer
			commandBuffers[index].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
			commandBuffers[index].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[index], 0, nullptr);

			for (auto model : renderData->getModels()) {
				vk::Buffer vertexBuffers[] = { model->mesh._vertexBuffer._buffer };
				vk::DeviceSize offsets[] = { 0 };
				commandBuffers[index].bindVertexBuffers(0, 1, vertexBuffers, offsets);

				commandBuffers[index].bindIndexBuffer(model->mesh._indexBuffer._buffer, 0, vk::IndexType::eUint32);

				commandBuffers[index].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, 1, &model->material.textureSet, 0, nullptr);

				MeshPushConstants constants = model->transformMatrix;
				commandBuffers[index].pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(MeshPushConstants), &constants);
				
				commandBuffers[index].drawIndexed(static_cast<uint32_t>(model->mesh._indices.size()), 1, 0, 0, 0);
			}
		}

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffers[index]);
		
		commandBuffers[index].endRenderPass();

		try {
			commandBuffers[index].end();
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("failed to record command buffer!") + err.what());
		}
}

void Renderer::createSyncObjects() {
	imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	imagesInFlight.resize(swapChainImages.size(), nullptr);

	vk::SemaphoreCreateInfo semaphoreInfo {};

	vk::FenceCreateInfo fenceInfo { .flags = vk::FenceCreateFlagBits::eSignaled };

	try {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			imageAvailableSemaphores[i] = device->device().createSemaphore(semaphoreInfo);
			renderFinishedSemaphores[i] = device->device().createSemaphore(semaphoreInfo);
			inFlightFences[i] = device->device().createFence(fenceInfo);
			mainDeletionQueue.push_function([&, i]() {
				device->device().destroySemaphore(renderFinishedSemaphores[i]);
				device->device().destroySemaphore(imageAvailableSemaphores[i]);
				device->device().destroyFence(inFlightFences[i]);
			});
		}
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create synchronization objects for a frame!") + err.what());
	}
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
	auto& ubo = renderData->getCameraProject(static_cast<float>(swapChainExtent.width), static_cast<float>(swapChainExtent.height));
    void* data;
    vmaMapMemory(device->allocator(), uniformBuffers[currentImage]._allocation, &data);
	{
		memcpy(data, &ubo, sizeof(ubo));
	}
    vmaUnmapMemory(device->allocator(), uniformBuffers[currentImage]._allocation);
}

void Renderer::drawFrame() {

	while (device->device().waitForFences(inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max())
		== vk::Result::eTimeout) {
		// Wait
	}

	uint32_t imageIndex;
	try {
		const auto acquired = 
			device->device().acquireNextImageKHR(swapChain,
			std::numeric_limits<uint64_t>::max(), 
				imageAvailableSemaphores[currentFrame], 
				nullptr);
		
		switch(acquired.result) {
			case vk::Result::eSuccess:
			case vk::Result::eSuboptimalKHR: // Swap chain is still valid, but surface properties are a miss match
				imageIndex = acquired.value; 
				break;
			case vk::Result::eErrorOutOfDateKHR:
				recreateSwapchain();

				return;
			default:
				throw std::runtime_error("failed to acquire swap chain image!");
		}
		
		
	}
	catch (vk::OutOfDateKHRError err) { // https://github.com/KhronosGroup/Vulkan-Hpp/issues/599
		recreateSwapchain();
		return;
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to acquire swap chain image!") + err.what());
	}

	if (imagesInFlight[imageIndex]) {
		while (device->device().waitForFences(imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max())
			== vk::Result::eTimeout) {
			// Wait
		}
	}
	
	imagesInFlight[imageIndex] = inFlightFences[currentFrame];
	
	ImGui::Render();

	vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

	updateUniformBuffer(imageIndex);
	
	recordCommandBuffer(imageIndex);

	vk::SubmitInfo submitInfo {

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,

		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[imageIndex],

		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,

	};

	device->device().resetFences(inFlightFences[currentFrame]);

	try {
		device->graphicsQueue().submit(submitInfo, inFlightFences[currentFrame]);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to submit draw command buffer") + err.what());

	}

	vk::SwapchainKHR swapChains[] = { swapChain };
	const vk::PresentInfoKHR presentInfo {

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = signalSemaphores,

		.swapchainCount = 1,
		.pSwapchains = swapChains,
		.pImageIndices = &imageIndex,
	};

	try {
		switch (device->presentQueue().presentKHR(presentInfo)) {
		case vk::Result::eSuccess:
			if (frameBufferResizePending) {
				recreateSwapchain();
				frameBufferResizePending = false;
			}
			break;
		case vk::Result::eSuboptimalKHR:
		case vk::Result::eErrorOutOfDateKHR: // What we would like to do :) But it's actually an exception
			recreateSwapchain();
			break;
		default:
			throw std::runtime_error("failed to present swap chain image!");  // an unexpected result is returned!
		}
	} catch (vk::OutOfDateKHRError& err) {
		std::cerr << "Recreating Swapchain: " << err.what() << "\n";
		recreateSwapchain();		
	}
	

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup() {
	swapChainDeletionQueue.flush();
	mainDeletionQueue.flush();
}

void Renderer::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels) {
	vk::ImageMemoryBarrier barrier {
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = vk::ImageSubresourceRange {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	} else {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	}
	else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	}
	else {
		throw std::invalid_argument("Unsupported layout transistion!");
	}

	device->immediateSubmit([&](auto cmd){
		cmd.pipelineBarrier(
			sourceStage, destinationStage, 
			vk::DependencyFlags(), 
			0, nullptr, 
			0, nullptr, 
			1, &barrier
		);
	});
}
