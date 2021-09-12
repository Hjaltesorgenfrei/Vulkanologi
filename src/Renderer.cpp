#include "Renderer.h"
#include "Vertice.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include <algorithm>
#include <fstream>
#include <set>
#include <map>
#include <stdexcept>
#include <iostream>


QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices;

	auto queueFamilies = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		// documentation of VkQueueFamilyProperties states that "Each queue family must support at least one queue".
		
		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
			indices.graphicsFamily = i;
		}

		if (device.getSurfaceSupportKHR(i, surface)) {
			indices.presentFamily = i;
		}

		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer && !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
			indices.transferFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}

	return indices;
}

VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}


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

Renderer::Renderer(std::shared_ptr<WindowWrapper>& window) {
	this->window = window;
	try {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
		createSyncObjects();
		createResizeCallback();
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

void Renderer::createInstance() {
	vk::ApplicationInfo appInfo {
		.pApplicationName = "Hello Triangle",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_1
	};



	vk::InstanceCreateInfo createInfo {

		.pApplicationInfo = &appInfo,
	};

	// Enable validation of best practices
	if (enableValidationLayers) {
		vk::ValidationFeatureEnableEXT enables[] = { vk::ValidationFeatureEnableEXT::eBestPractices };
		vk::ValidationFeaturesEXT features = {
			.sType = vk::StructureType::eValidationFeaturesEXT,
			.enabledValidationFeatureCount = 1,
			.pEnabledValidationFeatures = enables
		};
		createInfo.pNext = &features;
	}


	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	const auto supportedExtensions = vk::enumerateInstanceExtensionProperties();

	if (!validateExtensions(extensions, supportedExtensions)) {
		throw std::runtime_error("Required GLFW extension was not supported by Vulkan Driver!");
	}

	//Validate and enable validations Layers
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		createInfo.pNext = static_cast<vk::DebugUtilsMessengerCreateInfoEXT*>(&debugCreateInfo);
	}
	else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	try {
		instance = vk::createInstanceUnique(createInfo, nullptr);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create Vulkan Instance!") + err.what());
	}

}

std::vector<const char*> Renderer::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

bool Renderer::validateExtensions(std::vector<const char*> extensions,
	std::vector<vk::ExtensionProperties> supportedExtensions) {
	for (const auto& extension : extensions) {
		bool extensionFound = false;

		for (const auto& supportedExtension : supportedExtensions) {
			if (strcmp(extension, supportedExtension.extensionName) == 0) {
				extensionFound = true;
				break;
			}
		}

		if (!extensionFound) {
			return false;
		}
	}

	return true;
}

bool Renderer::checkValidationLayerSupport() const {
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}

VkBool32 Renderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) {

	//std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void Renderer::setupDebugMessenger() {
	if constexpr (!enableValidationLayers) return;

	const vk::DebugUtilsMessengerCreateInfoEXT createInfo = {
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
			| vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
			| vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
			| vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		.pfnUserCallback = debugCallback // I don't know if this works, it needs to be tested, as the types are not the same anymore
	};

	if (createDebugUtilsMessengerEXT(*instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo), nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
}

void Renderer::createSurface() {
	VkSurfaceKHR rawSurface;

	if (glfwCreateWindowSurface(*instance, window->getGLFWwindow(), nullptr, &rawSurface) != VK_SUCCESS) { // Ugly c :(
		throw std::runtime_error("Failed to create window surface!");
	}

	surface = rawSurface;
}

void Renderer::pickPhysicalDevice() {
	auto devices = instance->enumeratePhysicalDevices();

	if (devices.empty()) {
		throw std::runtime_error("Failed to find GPUs with Vulkan Support!");
	}

	//Get the score of devices
	std::multimap<int, vk::PhysicalDevice> candidates;

	for (const auto& device : devices) {
		int score = rateDeviceSuitability(device);
		candidates.insert(std::make_pair(score, device));
	}

	//Check if any suitable candidate was found
	if (candidates.rbegin()->first > 0) {
		physicalDevice = candidates.rbegin()->second;
	}
	else {
		throw std::runtime_error("Failed to find suitable GPU!");
	}
}

int Renderer::rateDeviceSuitability(vk::PhysicalDevice device) {
	auto deviceProperties = device.getProperties();
	auto deviceFeatures = device.getFeatures();

	int score = 0;

	// Discrete gpu has a significant performance advantage
	if (deviceProperties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
		score += 1000;
	}

	score += deviceProperties.limits.maxImageDimension2D;

	if (!deviceFeatures.geometryShader) {
		return 0;
	}

	// Queues supporting graphics and required plugins needs to be found
	QueueFamilyIndices indices = findQueueFamilies(device, surface);
	if (!indices.isComplete()) {
		return 0;
	}

	if (!checkDeviceExtensionSupport(device)) {
		return 0;
	}

	// Check if the device has a adequate swap chain 
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
	if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
		return 0;
	}

	return score;
}

bool Renderer::checkDeviceExtensionSupport(const vk::PhysicalDevice physicalDevice) {
	auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

SwapChainSupportDetails Renderer::querySwapChainSupport(vk::PhysicalDevice device) {
	SwapChainSupportDetails details;

	details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
	details.formats = device.getSurfaceFormatsKHR(surface);
	details.presentModes = device.getSurfacePresentModesKHR(surface);

	return details;
}

void Renderer::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(), indices.transferFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo {
			.queueFamilyIndex = queueFamily,
			.queueCount = 1,
			.pQueuePriorities = &queuePriority
		};

		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures {};

	vk::DeviceCreateInfo createInfo {
		.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
		.pQueueCreateInfos = queueCreateInfos.data(),
		.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
		.ppEnabledExtensionNames = deviceExtensions.data(),
		.pEnabledFeatures = &deviceFeatures
	};

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	try {
		device = physicalDevice.createDeviceUnique(createInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create logical device!") + err.what());
	}
	graphicsQueueIndex = indices.graphicsFamily.value();
	transferQueueIndex = indices.transferFamily.value();
	device->getQueue(indices.graphicsFamily.value(), 0, &graphicsQueue);
	device->getQueue(indices.presentFamily.value(), 0, &presentQueue);
	device->getQueue(indices.transferFamily.value(), 0, &transferQueue);
}

void Renderer::createSwapChain() {
	const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

	const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
	const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
	const vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	vk::SwapchainCreateInfoKHR createInfo {
		.surface = surface,
		.minImageCount = imageCount,
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = extent,
		.imageArrayLayers = 1,
		.imageUsage = vk::ImageUsageFlagBits::eColorAttachment
	};


	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);
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
		swapChain = device->createSwapchainKHR(createInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to create swap chain!") + err.what());
	}

	swapChainImages = device->getSwapchainImagesKHR(swapChain);

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void Renderer::cleanupSwapChain() {
	for (const auto framebuffer : swapChainFramebuffers) {
		device->destroyFramebuffer(framebuffer);
	}

	device->freeCommandBuffers(commandPool, commandBuffers);

	device->destroyPipeline(graphicsPipeline);
	device->destroyPipelineLayout(pipelineLayout);
	device->destroyRenderPass(renderPass);

	for (const auto imageView : swapChainImageViews) {
		device->destroyImageView(imageView);
	}

	device->destroySwapchainKHR(swapChain);

	for(size_t i = 0; i < swapChainImages.size(); i++) {
		device->destroyBuffer(uniformBuffers[i], nullptr);
		device->freeMemory(uniformBuffersMemory[i], nullptr);
	}

	device->destroyDescriptorPool(descriptorPool, nullptr);
}

void Renderer::recreateSwapchain() {
	int width = 0, height = 0;
	auto glfwWindow = window->getGLFWwindow();
	glfwGetFramebufferSize(glfwWindow, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(glfwWindow, &width, &height);
		glfwWaitEvents();
	}
	
	device->waitIdle(); // Has to wait for rendering to finish before creating new swapchain. Better solutions exists.
	
	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	createDescriptorSets();
	createCommandBuffers();

	imagesInFlight.resize(swapChainImages.size(), nullptr);
	std::cout << "Recreated swapchain" << std::endl;
}

vk::SurfaceFormatKHR Renderer::chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace ==
			vk::ColorSpaceKHR::eAdobergbNonlinearEXT) {
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

void Renderer::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		vk::ImageViewCreateInfo createInfo {
			.image = swapChainImages[i],

			.viewType = vk::ImageViewType::e2D,
			.format = swapChainImageFormat,

			.components = {
				.r = vk::ComponentSwizzle::eIdentity,
				.g = vk::ComponentSwizzle::eIdentity,
				.b = vk::ComponentSwizzle::eIdentity,
				.a = vk::ComponentSwizzle::eIdentity
			},

			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};

		try {
			swapChainImageViews[i] = device->createImageView(createInfo);
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("failed to create image views!") + err.what());
		}
	}
}

void Renderer::createRenderPass() {
	vk::AttachmentDescription colorAttachment {
		.format = swapChainImageFormat,
		.samples = vk::SampleCountFlagBits::e1,
		.loadOp = vk::AttachmentLoadOp::eClear,
		.storeOp = vk::AttachmentStoreOp::eStore,
		.stencilLoadOp = vk::AttachmentLoadOp::eDontCare,
		.stencilStoreOp = vk::AttachmentStoreOp::eDontCare,
		.initialLayout = vk::ImageLayout::eUndefined,
		.finalLayout = vk::ImageLayout::ePresentSrcKHR
	};


	vk::AttachmentReference colorAttachmentRef {
		.attachment = 0,
		.layout = vk::ImageLayout::eColorAttachmentOptimal
	};

	vk::SubpassDescription subpass {
		.pipelineBindPoint = vk::PipelineBindPoint::eGraphics,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef
	};

	vk::SubpassDependency dependency {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
		.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput,
		.dstAccessMask = vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
	};

	const vk::RenderPassCreateInfo renderPassInfo {
		.attachmentCount = 1,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency
	};

	try {
		renderPass = device->createRenderPass(renderPassInfo);
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
		.stageFlags = vk::ShaderStageFlagBits::eVertex,
		.pImmutableSamplers = nullptr
	};

	vk::DescriptorSetLayoutCreateInfo layoutInfo {
		.bindingCount = 1,
		.pBindings = &uboLayoutBinding
	};

	if(device->createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSeyLayout) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
	
}

void Renderer::createGraphicsPipeline() {
	auto vertShaderCode = readFile("src/shaders/vert.spv");
	auto fragShaderCode = readFile("src/shaders/frag.spv");

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
		.rasterizationSamples = vk::SampleCountFlagBits::e1,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f,
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


	vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSeyLayout,
		.pushConstantRangeCount = 0,
	};


	try {
		pipelineLayout = device->createPipelineLayout(pipelineLayoutInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to create pipeline layout!") + err.what());
	}


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
		.pColorBlendState = &colorBlending,

		// A vulkan handle, not a struct pointer
		.layout = pipelineLayout,

		.renderPass = renderPass,
		.subpass = 0,

		.basePipelineHandle = nullptr,
		.basePipelineIndex = -1,
	};

	auto result = device->createGraphicsPipeline(nullptr, pipelineInfo);
	if (result.result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	graphicsPipeline = result.value;

	device->destroyShaderModule(vertShaderModule);
	device->destroyShaderModule(fragShaderModule);
}

vk::ShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
	const vk::ShaderModuleCreateInfo createInfo {
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};

	vk::ShaderModule shaderModule;
	try {
		shaderModule = device->createShaderModule(createInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create shader module!") + err.what());
	}

	return shaderModule;
}

void Renderer::createFramebuffers() {
	swapChainFramebuffers.resize(swapChainImageViews.size());
	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vk::ImageView attachments[] = {
			swapChainImageViews[i]
		};

		vk::FramebufferCreateInfo framebufferInfo {
			.renderPass = renderPass,
			.attachmentCount = 1,
			.pAttachments = attachments,
			.width = swapChainExtent.width,
			.height = swapChainExtent.height,
			.layers = 1,
		};

		try {
			swapChainFramebuffers[i] = device->createFramebuffer(framebufferInfo);
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("Failed to create framebuffer") + err.what());
		}
	}
}

void Renderer::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
	const vk::CommandPoolCreateInfo poolInfo {
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
	};

	try {
		commandPool = device->createCommandPool(poolInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create graphics command pool!") + err.what());
	}

	const vk::CommandPoolCreateInfo transferPoolInfo {
		.queueFamilyIndex = queueFamilyIndices.transferFamily.value(),
	};

	try {
		transferCommandPool = device->createCommandPool(transferPoolInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create graphics command pool!") + err.what());
	}
}

uint32_t Renderer::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const {
	vk::PhysicalDeviceMemoryProperties memoryProperties;
	physicalDevice.getMemoryProperties(&memoryProperties);

	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
		if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

void Renderer::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
	vk::Buffer& buffer, vk::DeviceMemory& bufferMemory) {

	std::array<uint32_t, 2> allowedQueueIndices { graphicsQueueIndex, transferQueueIndex };
	vk::BufferCreateInfo bufferInfo {
		.size = size,
		.usage = usage,
		.sharingMode = vk::SharingMode::eConcurrent,
		.queueFamilyIndexCount = static_cast<uint32_t>(allowedQueueIndices.size()),
		.pQueueFamilyIndices = allowedQueueIndices.data()
	};

	if (device->createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create vertex buffer!");
	}

	vk::MemoryRequirements memoryRequirements;
	device->getBufferMemoryRequirements(buffer, &memoryRequirements);

	vk::MemoryAllocateInfo allocateInfo {
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties)
	};

	if (device->allocateMemory(&allocateInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}

	// If memory allocation was successful we can bind it to the buffer
	device->bindBufferMemory(buffer, bufferMemory, 0);
}

void Renderer::createIndexBuffer() {
	vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;
	createBuffer(
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		stagingBuffer,
		stagingBufferMemory
	);

	void* data = device->mapMemory(stagingBufferMemory, 0, bufferSize);
	memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
	device->unmapMemory(stagingBufferMemory);

	createBuffer(
		bufferSize,
		vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		indexBuffer,
		indexBufferMemory
	);

	copyBuffer(stagingBuffer, indexBuffer, bufferSize);
	device->destroyBuffer(stagingBuffer, nullptr);
	device->freeMemory(stagingBufferMemory, nullptr);
}

void Renderer::createVertexBuffer() {
	vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	vk::Buffer stagingBuffer;
	vk::DeviceMemory stagingBufferMemory;

	createBuffer(
		bufferSize,
		vk::BufferUsageFlagBits::eTransferSrc,
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		stagingBuffer,
		stagingBufferMemory
	);
	
	void* data = device->mapMemory(stagingBufferMemory, 0, bufferSize);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
	device->unmapMemory(stagingBufferMemory);

	createBuffer(
		bufferSize,
		vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
		vk::MemoryPropertyFlagBits::eDeviceLocal,
		vertexBuffer,
		vertexBufferMemory
	);

	copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
	device->destroyBuffer(stagingBuffer, nullptr);
	device->freeMemory(stagingBufferMemory, nullptr);
}

void Renderer::createUniformBuffers() {
	vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

	uniformBuffers.resize(swapChainImages.size());
	uniformBuffersMemory.resize(swapChainImages.size());

	for(size_t i = 0; i < swapChainImages.size(); i++) {
		createBuffer(
			bufferSize, 
			vk::BufferUsageFlagBits::eUniformBuffer, 
			vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible, 
			uniformBuffers[i], 
			uniformBuffersMemory[i]
		);
	}
}

void Renderer::createDescriptorPool() {
	vk::DescriptorPoolSize poolSize {
		.descriptorCount = static_cast<uint32_t>(swapChainImages.size())
	};

	vk::DescriptorPoolCreateInfo poolInfo {
		.maxSets = static_cast<uint32_t>(swapChainImages.size()),
		.poolSizeCount = 1,
		.pPoolSizes = &poolSize,
	};

	if(device->createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create descriptor pool");
	}
}

void Renderer::createDescriptorSets() {
	std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSeyLayout);
	vk::DescriptorSetAllocateInfo allocInfo {
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size()),
		.pSetLayouts = layouts.data()
	};

	descriptorSets.resize(swapChainImages.size());
	if(device->allocateDescriptorSets(&allocInfo, descriptorSets.data()) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create Descriptor sets!");
	}

	for(size_t i = 0; i < swapChainImages.size(); i++) {
		vk::DescriptorBufferInfo bufferInfo {
			.buffer = uniformBuffers[i],
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		vk::WriteDescriptorSet descriptorWrite {
			.dstSet = descriptorSets[i],
			.dstBinding = 0,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.pImageInfo = nullptr,
			.pBufferInfo = &bufferInfo,
			.pTexelBufferView = nullptr
		};

		device->updateDescriptorSets(1, &descriptorWrite, 0, nullptr);
	}
}

void Renderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	vk::CommandBufferAllocateInfo allocateInfo {
		.commandPool = transferCommandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = 1
	};

	vk::CommandBuffer commandBuffer;
	if(device->allocateCommandBuffers(&allocateInfo, &commandBuffer) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to copy command buffer!");
	}

	vk::CommandBufferBeginInfo beginInfo {
		.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
	};
	commandBuffer.begin(beginInfo);
	{
		vk::BufferCopy copyRegion{
			.srcOffset = 0,
			.dstOffset = 0,
			.size = size
		};
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);
	}
	commandBuffer.end();

	vk::SubmitInfo submitInfo{
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer
	};
	
	if (transferQueue.submit(1, &submitInfo, nullptr) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to copy command buffer!");
	}
	transferQueue.waitIdle();

	device->freeCommandBuffers(transferCommandPool, 1, &commandBuffer);
}

void Renderer::createCommandBuffers() {
	commandBuffers.resize(swapChainFramebuffers.size());

	vk::CommandBufferAllocateInfo allocateInfo {
		.commandPool = commandPool,
		.level = vk::CommandBufferLevel::ePrimary,
		.commandBufferCount = static_cast<uint32_t>(commandBuffers.size()),
	};

	try {
		commandBuffers = device->allocateCommandBuffers(allocateInfo);
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to allocate command buffers") + err.what());
	}

	// Record to command buffers
	for (size_t i = 0; i < commandBuffers.size(); i++) {
		vk::CommandBufferBeginInfo beginInfo {};

		try {
			commandBuffers[i].begin(beginInfo);
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("failed to begin recording command buffer!") + err.what());
		}

		vk::RenderPassBeginInfo renderPassInfo {
			.renderPass = renderPass,
			.framebuffer = swapChainFramebuffers[i],
			.renderArea = {
				.offset = {0, 0},
				.extent = swapChainExtent,
			},
			.clearValueCount = 1
		};
		vk::ClearValue clearColor = { std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } };
		renderPassInfo.pClearValues = &clearColor;

		commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
		{
			//Add commands to buffer
			commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

			vk::Buffer vertexBuffers[] = { vertexBuffer };
			vk::DeviceSize offsets[] = { 0 };
			commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);

			commandBuffers[i].bindIndexBuffer(indexBuffer, 0, vk::IndexType::eUint16);

			commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[i], 0, nullptr);
			
			commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		}
		commandBuffers[i].endRenderPass();

		try {
			commandBuffers[i].end();
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("failed to record command buffer!") + err.what());
		}
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
			imageAvailableSemaphores[i] = device->createSemaphore(semaphoreInfo);
			renderFinishedSemaphores[i] = device->createSemaphore(semaphoreInfo);
			inFlightFences[i] = device->createFence(fenceInfo);
		}
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create synchronization objects for a frame!") + err.what());
	}
}

void Renderer::updateUniformBuffer(uint32_t currentImage) {
	static auto startTime = std::chrono::high_resolution_clock::now();

	const auto currentTime = std::chrono::high_resolution_clock::now();
	auto time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

	UniformBufferObject ubo {
		.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
		.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / static_cast<float>(swapChainExtent.height), 0.1f, 10.0f)
	};

	ubo.proj[1][1] *= -1; // GLM was originally designed for OpenGL, where the Y coordinate of the clip coordinates is inverted.

	void* data = device->mapMemory(uniformBuffersMemory[currentImage], 0, sizeof(ubo));
	{
		memcpy(data, &ubo, sizeof(ubo));
	}
	device->unmapMemory(uniformBuffersMemory[currentImage]);
}

void Renderer::drawFrame() {

	while (device->waitForFences(inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max())
		== vk::Result::eTimeout) {
		// Wait
	}

	uint32_t imageIndex;
	try {
		const auto acquired = 
			device->acquireNextImageKHR(swapChain, 
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
		while (device->waitForFences(imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max())
			== vk::Result::eTimeout) {
			// Wait
		}
	}

	imagesInFlight[imageIndex] = inFlightFences[currentFrame];


	vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
	vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
	vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };

	updateUniformBuffer(imageIndex);
	
	vk::SubmitInfo submitInfo {

		.waitSemaphoreCount = 1,
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,

		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffers[imageIndex],

		.signalSemaphoreCount = 1,
		.pSignalSemaphores = signalSemaphores,

	};

	device->resetFences(inFlightFences[currentFrame]);

	try {
		graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);
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
		switch (presentQueue.presentKHR(presentInfo)) {
		case vk::Result::eSuccess:
			if (framebufferResized) {
				recreateSwapchain();
				framebufferResized = false;
			}
			break;
		case vk::Result::eSuboptimalKHR:
		case vk::Result::eErrorOutOfDateKHR: // What we would like to do :) But it's actually an exception
			recreateSwapchain();
			break;
		default:
			throw std::runtime_error("failed to present swap chain image!");  // an unexpected result is returned!
		}
	} catch (vk::OutOfDateKHRError err) {
		recreateSwapchain();		
	}
	

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup() {
	// Destruction of device and instance is handled by unique

	device->waitIdle();

	for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
		device->destroySemaphore(renderFinishedSemaphores[i]);
		device->destroySemaphore(imageAvailableSemaphores[i]);
		device->destroyFence(inFlightFences[i]);
	}

	cleanupSwapChain();

	device->destroyDescriptorSetLayout(descriptorSeyLayout, nullptr);

	device->destroyBuffer(indexBuffer, nullptr);
	device->freeMemory(indexBufferMemory, nullptr);
	
	device->destroyBuffer(vertexBuffer, nullptr);
	device->freeMemory(vertexBufferMemory, nullptr);
	
	device->destroyCommandPool(commandPool);
	device->destroyCommandPool(transferCommandPool);

	if (enableValidationLayers) {
		destroyDebugUtilsMessengerEXT(instance.get(), debugMessenger, nullptr);
	}

	instance->destroySurfaceKHR(surface);


}

void Renderer::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto* const render = static_cast<Renderer*>(glfwGetWindowUserPointer(window));
	render->framebufferResized = true;
}

void Renderer::createResizeCallback() {
	glfwSetWindowUserPointer(window->getGLFWwindow(), this);
	glfwSetFramebufferSizeCallback(window->getGLFWwindow(), framebufferResizeCallback);
}

