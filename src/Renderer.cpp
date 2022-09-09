#include "Renderer.h"
#include "Mesh.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

const std::string TEXTURE_PATH = "resources/viking_room.png";

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

vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice) {
    auto physicalDeviceProperties = physicalDevice.getProperties();

    auto counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }

    return vk::SampleCountFlagBits::e1;
}

Renderer::Renderer(std::shared_ptr<WindowWrapper>& window, std::shared_ptr<RenderData>& renderData) {
	this->window = window;
	this->renderData = renderData;
	try {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createAllocator();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipelineLayout();
		createGraphicsPipeline();
		createCommandPool();
        createUploadContext();
		initImgui();
		createColorResources();
		createDepthResources();
		createFramebuffers();
        createTextureImage();
		createTextureImageView();
		createTextureSampler();
		uploadMeshes();
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
		vk::ValidationFeatureEnableEXT enables[] = { 
			vk::ValidationFeatureEnableEXT::eBestPractices, 
			vk::ValidationFeatureEnableEXT::eSynchronizationValidation
		};
		vk::ValidationFeaturesEXT features = {
			.sType = vk::StructureType::eValidationFeaturesEXT,
			.enabledValidationFeatureCount = 2,
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
		instance = vk::createInstance(createInfo, nullptr);
		mainDeletionQueue.push_function([&]() {
			instance.destroy();
        });
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

    if (strncmp(pCallbackData->pMessage, "Device Extension:", strlen("Device Extension:")) == 0) {
        return VK_TRUE; // Don't spam with loaded layers
    }

	std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

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

	if (createDebugUtilsMessengerEXT(instance, reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo), nullptr, &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
	mainDeletionQueue.push_function([&]() {
		destroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
	});
}

void Renderer::createSurface() {
	VkSurfaceKHR rawSurface;

	if (glfwCreateWindowSurface(instance, window->getGLFWwindow(), nullptr, &rawSurface) != VK_SUCCESS) { // Ugly c :(
		throw std::runtime_error("Failed to create window surface!");
	}

	surface = rawSurface;
	mainDeletionQueue.push_function([&]() {
		instance.destroySurfaceKHR(surface);
	});
}

void Renderer::pickPhysicalDevice() {
	auto devices = instance.enumeratePhysicalDevices();

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
		msaaSamples = getMaxUsableSampleCount(physicalDevice);
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

	if (deviceFeatures.samplerAnisotropy == VK_FALSE) {
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

	vk::PhysicalDeviceFeatures deviceFeatures {
		.sampleRateShading = VK_TRUE,
		.samplerAnisotropy = VK_TRUE
	};

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
		device = physicalDevice.createDevice(createInfo);
		mainDeletionQueue.push_function([&]() {
			device.destroy();
        });
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create logical device!") + err.what());
	}
	graphicsQueueIndex = indices.graphicsFamily.value();
	transferQueueIndex = indices.transferFamily.value();
	device.getQueue(indices.graphicsFamily.value(), 0, &graphicsQueue);
	device.getQueue(indices.presentFamily.value(), 0, &presentQueue);
	device.getQueue(indices.transferFamily.value(), 0, &transferQueue);
}

void Renderer::createAllocator() {
	VmaAllocatorCreateInfo allocatorInfo {
		.physicalDevice = physicalDevice,
		.device = device,
		.instance = instance
	};
	vmaCreateAllocator(&allocatorInfo, &allocator);
	mainDeletionQueue.push_function([&]() {
		vmaDestroyAllocator(allocator);
	});
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
		swapChain = device.createSwapchainKHR(createInfo);
		swapChainDeletionQueue.push_function([&]() {
			device.destroySwapchainKHR(swapChain);
		});
	}
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("failed to create swap chain!") + err.what());
	}

	swapChainImages = device.getSwapchainImagesKHR(swapChain);

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
	
	device.waitIdle(); // Has to wait for rendering to finish before creating new swapchain. Better solutions exists.
	
	swapChainDeletionQueue.flush();

	createSwapChain();
	createImageViews();
	createRenderPass();
	createGraphicsPipeline();
	createColorResources();
	createDepthResources();
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

	imguiPool = device.createDescriptorPool(pool_info);


	// 2: initialize imgui library

	//this initializes the core structures of imgui
	ImGui::CreateContext();

	//this initializes imgui for Glfw
	ImGui_ImplGlfw_InitForVulkan(window->getGLFWwindow(), true);

	//this initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = device;
	init_info.Queue = graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = static_cast<VkSampleCountFlagBits>(msaaSamples);

	ImGui_ImplVulkan_Init(&init_info, renderPass);

	//execute a gpu command to upload imgui font textures
	immediateSubmit([&](auto cmd){
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});

	//clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();
	mainDeletionQueue.push_function([&]() {
		device.destroyDescriptorPool(imguiPool);
		ImGui_ImplVulkan_Shutdown();
	});
}

void Renderer::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());
	for (size_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, vk::ImageAspectFlagBits::eColor, 1);
		swapChainDeletionQueue.push_function([&, i]() {
			device.destroyImageView(swapChainImageViews[i]);
		});
	}
}

void Renderer::createRenderPass() {
	vk::AttachmentDescription colorAttachment {
		.format = swapChainImageFormat,
		.samples = msaaSamples,
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
		.samples = msaaSamples,
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
		renderPass = device.createRenderPass(renderPassInfo);
		swapChainDeletionQueue.push_function([&]() {
			device.destroyRenderPass(renderPass);
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
		.stageFlags = vk::ShaderStageFlagBits::eVertex,
		.pImmutableSamplers = nullptr
	};

	vk::DescriptorSetLayoutBinding samplerLayoutBinding { 
		.binding = 1,
		.descriptorType = vk::DescriptorType::eCombinedImageSampler,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eFragment,
		.pImmutableSamplers = nullptr
	};

	std::array<vk::DescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

	vk::DescriptorSetLayoutCreateInfo layoutInfo {
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	if(device.createDescriptorSetLayout(&layoutInfo, nullptr, &descriptorSetLayout) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create descriptor set layout");
	}
	mainDeletionQueue.push_function([&]() {
		device.destroyDescriptorSetLayout(descriptorSetLayout);
	});
}


void Renderer::createGraphicsPipelineLayout() {
	 vk::PushConstantRange pushConstantRange {
        .stageFlags = vk::ShaderStageFlagBits::eVertex,
        .offset = 0,
        .size = sizeof(MeshPushConstants)
    };

	vk::PipelineLayoutCreateInfo pipelineLayoutInfo {
		.setLayoutCount = 1,
		.pSetLayouts = &descriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
	};

	try {
		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		mainDeletionQueue.push_function([&]() {
			device.destroyPipelineLayout(pipelineLayout);
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
		.rasterizationSamples = msaaSamples,
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

	auto result = device.createGraphicsPipeline(nullptr, pipelineInfo);
	if (result.result != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	graphicsPipeline = result.value;
	swapChainDeletionQueue.push_function([&]() {
		device.destroyPipeline(graphicsPipeline);
	});

	device.destroyShaderModule(vertShaderModule);
	device.destroyShaderModule(fragShaderModule);
}

vk::ShaderModule Renderer::createShaderModule(const std::vector<char>& code) {
	const vk::ShaderModuleCreateInfo createInfo {
		.codeSize = code.size(),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};

	vk::ShaderModule shaderModule;
	try {
		shaderModule = device.createShaderModule(createInfo);
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
			swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
			swapChainDeletionQueue.push_function([&, i]() {
				device.destroyFramebuffer(swapChainFramebuffers[i]);
			});
		}
		catch (vk::SystemError& err) {
			throw std::runtime_error(std::string("Failed to create framebuffer") + err.what());
		}
	}
}

void Renderer::createCommandPool() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);
	const vk::CommandPoolCreateInfo poolInfo {
		.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
		.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
	};

    const vk::CommandPoolCreateInfo transferPoolInfo {
            .queueFamilyIndex = queueFamilyIndices.transferFamily.value(),
    };

	try {
		commandPool = device.createCommandPool(poolInfo);
        transferCommandPool = device.createCommandPool(transferPoolInfo);
		mainDeletionQueue.push_function([&]() {
			device.destroyCommandPool(commandPool);
			device.destroyCommandPool(transferCommandPool);
        });
    }
	catch (vk::SystemError& err) {
		throw std::runtime_error(std::string("Failed to create graphics command pools!") + err.what());
	}
}

void Renderer::createColorResources() {
	auto colorFormat = swapChainImageFormat;

	createImage(
		swapChainExtent.width, 
		swapChainExtent.height, 
		1, 
		msaaSamples, 
		colorFormat, 
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
		colorImage,
		colorImageMemory
	);
	colorImageView = createImageView(colorImage, colorFormat, vk::ImageAspectFlagBits::eColor, 1);
	swapChainDeletionQueue.push_function([&]() {
		device.destroyImageView(colorImageView);
		device.destroyImage(colorImage);
		device.freeMemory(colorImageMemory);
	});
}

void Renderer::createDepthResources() {
	auto depthFormat = findDepthFormat();
	createImage(
		swapChainExtent.width, 
		swapChainExtent.height, 
		1,
		msaaSamples,
		depthFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment,
		depthImage,
		depthImageMemory
	);
	depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth, 1);

	transitionImageLayout(depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal, 1);
	swapChainDeletionQueue.push_function([&]() {
		device.destroyImageView(depthImageView);
		device.destroyImage(depthImage);
		device.freeMemory(depthImageMemory);
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
		auto props = physicalDevice.getFormatProperties(format);

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

	if (device.createBuffer(&bufferInfo, nullptr, &buffer) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create vertex buffer!");
	}

	vk::MemoryRequirements memoryRequirements;
	device.getBufferMemoryRequirements(buffer, &memoryRequirements);

	vk::MemoryAllocateInfo allocateInfo {
		.allocationSize = memoryRequirements.size,
		.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties)
	};

	if (device.allocateMemory(&allocateInfo, nullptr, &bufferMemory) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to allocate vertex buffer memory!");
	}

	// If memory allocation was successful we can bind it to the buffer
	device.bindBufferMemory(buffer, bufferMemory, 0);
}

void Renderer::createTextureImage() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if(!pixels) {
        throw std::runtime_error("Failed to load texture image!");
    }

    vk::Buffer stagingBuffer;
    vk::DeviceMemory stagingBufferMemory;
    createBuffer(
        imageSize,
        vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
        stagingBuffer,
        stagingBufferMemory
    );


    void* data = device.mapMemory(stagingBufferMemory, 0, imageSize);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    device.unmapMemory(stagingBufferMemory);

    stbi_image_free(pixels);

    createImage(
        texWidth,
        texHeight,
		mipLevels,
		vk::SampleCountFlagBits::e1,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
        textureImage,
        textureImageMemory
    );
	transitionImageLayout(
		textureImage, 
		vk::Format::eR8G8B8A8Srgb, 
		vk::ImageLayout::eUndefined, 
		vk::ImageLayout::eTransferDstOptimal,
		mipLevels
	);
	copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	device.destroyBuffer(stagingBuffer);
	device.freeMemory(stagingBufferMemory);

	generateMipmaps(textureImage, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, mipLevels);
	mainDeletionQueue.push_function([&]() {
		device.destroyImage(textureImage);
		device.freeMemory(textureImageMemory);
	});
}

void Renderer::createImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags,
                           vk::Image& image, vk::DeviceMemory& imageMemory) {
    vk::ImageCreateInfo imageInfo {
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = vk::Extent3D {
            .width = static_cast<uint32_t>(width),
            .height = static_cast<uint32_t>(height),
            .depth = 1
        },
        .mipLevels = mipLevels,
        .arrayLayers = 1,
        .samples = numSamples,
        .tiling = tiling,
        .usage = flags,
        .sharingMode = vk::SharingMode::eExclusive,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    try {
        image = device.createImage(imageInfo);
    }
    catch (vk::SystemError& err) {
        throw std::runtime_error(std::string("failed to create image! ") + err.what());
    }

    auto memRequirements = device.getImageMemoryRequirements(image);
    vk::MemoryAllocateInfo allocInfo {
        .allocationSize = memRequirements.size,
        .memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    try {
        imageMemory = device.allocateMemory(allocInfo);
    }
    catch (vk::SystemError& err) {
        throw std::runtime_error(std::string("failed to create image memory! ") + err.what());
    }

    device.bindImageMemory(image, imageMemory, 0);
}

void Renderer::createTextureImageView() {
	textureImageView = createImageView(textureImage, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mipLevels);
	mainDeletionQueue.push_function([&]() {
		device.destroyImageView(textureImageView);
	});
}

void Renderer::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
	auto properties = physicalDevice.getFormatProperties(imageFormat);
	
	if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
		throw std::runtime_error("Texture image format does not support linear blitting!");
	}

	immediateSubmit([&](auto commandBuffer){
		vk::ImageMemoryBarrier barrier {
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			}
		};

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for(uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, 
				vk::DependencyFlags(),
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			vk::ImageBlit blit{
				.srcSubresource = {
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.mipLevel = i - 1,
					.baseArrayLayer = 0,
					.layerCount = 1
				},
				.dstSubresource = {
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.mipLevel = i,
					.baseArrayLayer = 0,
					.layerCount = 1
				}
			};
			blit.srcOffsets[0] = vk::Offset3D {0, 0, 0};
			blit.srcOffsets[1] = vk::Offset3D {mipWidth, mipHeight, 1};
			blit.dstOffsets[0] = vk::Offset3D {0, 0, 0};
			blit.dstOffsets[1] = vk::Offset3D {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
			
			commandBuffer.blitImage(
				image, vk::ImageLayout::eTransferSrcOptimal,
				image, vk::ImageLayout::eTransferDstOptimal,
				1, &blit,
				vk::Filter::eLinear
			);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, 
				vk::DependencyFlags(),
				0, nullptr,
				0, nullptr,
				1, &barrier
			);

			if(mipWidth > 1) mipWidth /= 2;
			if(mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, 
			vk::DependencyFlags(),
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	});
}

vk::ImageView Renderer::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels) {
	vk::ImageViewCreateInfo viewInfo{
		.image = image,
		.viewType = vk::ImageViewType::e2D,
		.format = format,
		.subresourceRange = vk::ImageSubresourceRange {
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1
		}
	};

	vk::ImageView imageView;
	if (device.createImageView(&viewInfo, nullptr, &imageView) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create image view!");
    }

	return imageView;
}

void Renderer::createTextureSampler() {
	auto properties = physicalDevice.getProperties();

	vk::SamplerCreateInfo samplerInfo{
		.magFilter = vk::Filter::eLinear,
		.minFilter = vk::Filter::eLinear,
		.mipmapMode = vk::SamplerMipmapMode::eLinear,
		.addressModeU = vk::SamplerAddressMode::eRepeat,
		.addressModeV = vk::SamplerAddressMode::eRepeat,
		.addressModeW = vk::SamplerAddressMode::eRepeat,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = VK_FALSE,
		.compareOp = vk::CompareOp::eAlways,
		.minLod = 0.0f,
		.maxLod = static_cast<float>(mipLevels),
		.borderColor = vk::BorderColor::eIntOpaqueBlack,
		.unnormalizedCoordinates = VK_FALSE,
	};

	if (device.createSampler(&samplerInfo, nullptr, &textureSampler) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
	mainDeletionQueue.push_function([&]() {
		device.destroySampler(textureSampler);
	});
}

void Renderer::uploadMeshes() {
	for (auto model : renderData->getModels()) {
        model->mesh._vertexBuffer = uploadBuffer(model->mesh._vertices, VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
        model->mesh._indexBuffer = uploadBuffer(model->mesh._indices, VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
		mainDeletionQueue.push_function([&, model]() {
			vmaDestroyBuffer(allocator, model->mesh._vertexBuffer._buffer, model->mesh._vertexBuffer._allocation);
			vmaDestroyBuffer(allocator, model->mesh._indexBuffer._buffer, model->mesh._indexBuffer._allocation);
        });
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

    if (vmaCreateBuffer(allocator, &stagingCreate, &stagingAlloc, &stagingBuffer, &stagingAllocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to uploadBuffer mesh vertices!");
    }

    void *data;
    vmaMapMemory(allocator, stagingAllocation, &data);
    {
        memcpy(data, meshData.data(), meshData.size() * sizeof(T));
    }
    vmaUnmapMemory(allocator, stagingAllocation);

    VkBufferCreateInfo bufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = meshData.size() * sizeof(T),
            .usage = usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT
    };

    VmaAllocationCreateInfo vmaallocInfo {
            .usage = VMA_MEMORY_USAGE_GPU_ONLY
    };

    VkBuffer buffer;

    AllocatedBuffer allocatedBuffer{};

    if (vmaCreateBuffer(allocator, &bufferCreateInfo, &vmaallocInfo, &buffer, &allocatedBuffer._allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to upload buffer!");
    }
    allocatedBuffer._buffer = buffer;

    copyBuffer(stagingBuffer, buffer, meshData.size() * sizeof(T));

    vmaDestroyBuffer(allocator, stagingBuffer, stagingAllocation);
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
        if (vmaCreateBuffer(allocator, &create, &allocCreate, &buffer, &allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to upload buffer!");
        }
        uniformBuffers[i]._buffer = buffer;
		uniformBuffers[i]._allocation = allocation;
		swapChainDeletionQueue.push_function([&, i]() {
			vmaDestroyBuffer(allocator, uniformBuffers[i]._buffer, uniformBuffers[i]._allocation);
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
		.maxSets = static_cast<uint32_t>(swapChainImages.size()),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data(),
	};

	if(device.createDescriptorPool(&poolInfo, nullptr, &descriptorPool) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create descriptor pool");
	}
	swapChainDeletionQueue.push_function([&]() {
		device.destroyDescriptorPool(descriptorPool, nullptr);
	});
}

void Renderer::createDescriptorSets() {
	std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
	vk::DescriptorSetAllocateInfo allocInfo {
		.descriptorPool = descriptorPool,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size()),
		.pSetLayouts = layouts.data()
	};

	descriptorSets.resize(swapChainImages.size());
	if(device.allocateDescriptorSets(&allocInfo, descriptorSets.data()) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create Descriptor sets!");
	}

	for(size_t i = 0; i < swapChainImages.size(); i++) {
		vk::DescriptorBufferInfo bufferInfo {
			.buffer = uniformBuffers[i]._buffer,
			.offset = 0,
			.range = sizeof(UniformBufferObject)
		};

		vk::DescriptorImageInfo imageInfo {
			.sampler = textureSampler,
			.imageView = textureImageView,
			.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal,
		};

		std::array<vk::WriteDescriptorSet, 2> descriptorWrites {
			vk::WriteDescriptorSet {
				.dstSet = descriptorSets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eUniformBuffer,
				.pBufferInfo = &bufferInfo,
			},
			vk::WriteDescriptorSet {
				.dstSet = descriptorSets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = vk::DescriptorType::eCombinedImageSampler,
				.pImageInfo = &imageInfo,
			}
		};

		device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

void Renderer::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size) {
	immediateSubmit([&](vk::CommandBuffer cmd){
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
		commandBuffers = device.allocateCommandBuffers(allocateInfo);
		swapChainDeletionQueue.push_function([&]() {
			device.freeCommandBuffers(commandPool, commandBuffers);
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

			for (auto model : renderData->getModels()) {
				vk::Buffer vertexBuffers[] = { model->mesh._vertexBuffer._buffer };
				vk::DeviceSize offsets[] = { 0 };
				commandBuffers[index].bindVertexBuffers(0, 1, vertexBuffers, offsets);

				commandBuffers[index].bindIndexBuffer(model->mesh._indexBuffer._buffer, 0, vk::IndexType::eUint32);

				commandBuffers[index].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, 1, &descriptorSets[index], 0, nullptr);

				MeshPushConstants constants = renderData->getPushConstants();
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
			imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
			renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
			inFlightFences[i] = device.createFence(fenceInfo);
			mainDeletionQueue.push_function([&, i]() {
				device.destroySemaphore(renderFinishedSemaphores[i]);
				device.destroySemaphore(imageAvailableSemaphores[i]);
				device.destroyFence(inFlightFences[i]);
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
    vmaMapMemory(allocator, uniformBuffers[currentImage]._allocation, &data);
	{
		memcpy(data, &ubo, sizeof(ubo));
	}
    vmaUnmapMemory(allocator, uniformBuffers[currentImage]._allocation);
}

void Renderer::drawFrame() {

	while (device.waitForFences(inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max())
		== vk::Result::eTimeout) {
		// Wait
	}

	uint32_t imageIndex;
	try {
		const auto acquired = 
			device.acquireNextImageKHR(swapChain, 
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
		while (device.waitForFences(imagesInFlight[imageIndex], VK_TRUE, std::numeric_limits<uint64_t>::max())
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

	device.resetFences(inFlightFences[currentFrame]);

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
	} catch (vk::OutOfDateKHRError& err) {
		std::cerr << "Recreating Swapchain: " << err.what() << "\n";
		recreateSwapchain();		
	}
	

	currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::cleanup() {
	device.waitIdle();
	swapChainDeletionQueue.flush();
	mainDeletionQueue.flush();
}

void Renderer::createUploadContext() {
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice, surface);

    const vk::CommandPoolCreateInfo uploadPoolInfo {
            .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value()
    };

    try {
        _uploadContext._commandPool = device.createCommandPool(uploadPoolInfo);

        vk::CommandBufferAllocateInfo uploadAllocateInfo {
                .commandPool = _uploadContext._commandPool,
                .level = vk::CommandBufferLevel::ePrimary,
                .commandBufferCount = static_cast<uint32_t>(1),
        };

        _uploadContext._commandBuffer = device.allocateCommandBuffers(uploadAllocateInfo)[0];

        vk::FenceCreateInfo uploadFenceInfo { };
        _uploadContext._uploadFence = device.createFence(uploadFenceInfo);
		mainDeletionQueue.push_function([&]() {
    		device.destroyFence(_uploadContext._uploadFence);
    		device.destroyCommandPool(_uploadContext._commandPool);
        });
    }
    catch (vk::SystemError& err) {
        throw std::runtime_error(std::string("Failed in creating upload context") + err.what());
    }
}

void Renderer::immediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function) {
	auto& commandBuffer = _uploadContext._commandBuffer;
    vk::CommandBufferBeginInfo beginInfo {
            .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    };

    commandBuffer.begin(beginInfo);
	function(commandBuffer);
	commandBuffer.end();

    vk::SubmitInfo submitInfo {
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer
    };
    auto& fence = _uploadContext._uploadFence;
    if (graphicsQueue.submit(1, &submitInfo, fence) != vk::Result::eSuccess) {
        throw std::runtime_error("Immediate submit failed!");
    }
    if (device.waitForFences(1, &fence, true, 99999999) != vk::Result::eSuccess) {
        throw std::runtime_error("Immediate submit failed!");
    }
    if (device.resetFences(1, &fence) != vk::Result::eSuccess) {
        throw std::runtime_error("Immediate submit failed!");
    }
    device.resetCommandPool(_uploadContext._commandPool);
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

	immediateSubmit([&](auto cmd){
		cmd.pipelineBarrier(
			sourceStage, destinationStage, 
			vk::DependencyFlags(), 
			0, nullptr, 
			0, nullptr, 
			1, &barrier
		);
	});
}

void Renderer::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {	
	vk::BufferImageCopy region {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = vk::ImageSubresourceLayers {
			.aspectMask = vk::ImageAspectFlagBits::eColor,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1
		},
		.imageOffset =  {0, 0, 0},
		.imageExtent = {width, height, 1}
	};

	immediateSubmit([&](auto cmd){
		cmd.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
	});
}
