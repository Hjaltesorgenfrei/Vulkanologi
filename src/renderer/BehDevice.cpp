#include "BehDevice.hpp"

#include <utility>

auto apiVersion = VK_API_VERSION_1_2;

BehDevice::BehDevice(std::shared_ptr<WindowWrapper> window) : window{std::move(window)} {
	createInstance();
	createSurface();
	pickPhysicalDevice();
	createLogicalDevice();
	setupDebugMessenger();
	createAllocator();
	createUploadContext();
}

BehDevice::~BehDevice() {
	_device.waitIdle();
	mainDeletionQueue.flush();
}

void BehDevice::immediateSubmit(std::function<void(vk::CommandBuffer cmd)> &&function) {
	auto &commandBuffer = _uploadContext._commandBuffer;
	vk::CommandBufferBeginInfo beginInfo{.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

	commandBuffer.begin(beginInfo);
	function(commandBuffer);
	commandBuffer.end();

	vk::SubmitInfo submitInfo{.commandBufferCount = 1, .pCommandBuffers = &commandBuffer};
	auto &fence = _uploadContext._uploadFence;
	if (_graphicsQueue.submit(1, &submitInfo, fence) != vk::Result::eSuccess) {
		throw std::runtime_error("Immediate submit failed!");
	}
	if (_device.waitForFences(1, &fence, true, 99999999) != vk::Result::eSuccess) {
		throw std::runtime_error("Immediate submit failed!");
	}
	if (_device.resetFences(1, &fence) != vk::Result::eSuccess) {
		throw std::runtime_error("Immediate submit failed!");
	}
	_device.resetCommandPool(_uploadContext._commandPool);
}

void BehDevice::createInstance() {
	vk::ApplicationInfo appInfo{.pApplicationName = "Hello Triangle",
								.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
								.pEngineName = "No Engine",
								.engineVersion = VK_MAKE_VERSION(1, 0, 0),
								.apiVersion = apiVersion};

	vk::InstanceCreateInfo createInfo{

		.pApplicationInfo = &appInfo,
	};

	// Enable validation of best practices
	if (enableValidationLayers) {
		vk::ValidationFeatureEnableEXT enables[] = {
			vk::ValidationFeatureEnableEXT::eGpuAssisted,
			vk::ValidationFeatureEnableEXT::eGpuAssistedReserveBindingSlot,
			vk::ValidationFeatureEnableEXT::eBestPractices,
			vk::ValidationFeatureEnableEXT::eDebugPrintf,
			vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
		};

		vk::ValidationFeaturesEXT features = {
			.sType = vk::StructureType::eValidationFeaturesEXT,
			.enabledValidationFeatureCount = static_cast<uint32_t>(std::size(enables)),
			.pEnabledValidationFeatures = enables};
		createInfo.pNext = &features;
	}

	auto extensions = getRequiredExtensions();
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.data();

	const auto supportedExtensions = vk::enumerateInstanceExtensionProperties();

	if (!validateExtensions(extensions, supportedExtensions)) {
		throw std::runtime_error("Required GLFW extension was not supported by Vulkan Driver!");
	}

	// Validate and enable validations Layers
	if (enableValidationLayers && !checkValidationLayerSupport()) {
		throw std::runtime_error("validation layers requested, but not available!");
	}

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();

		createInfo.pNext = static_cast<vk::DebugUtilsMessengerCreateInfoEXT *>(&debugCreateInfo);
	} else {
		createInfo.enabledLayerCount = 0;

		createInfo.pNext = nullptr;
	}

	try {
		_instance = vk::createInstance(createInfo, nullptr);
		mainDeletionQueue.push_function([&]() { _instance.destroy(); });
	} catch (vk::SystemError &err) {
		throw std::runtime_error(std::string("Failed to create Vulkan Instance!") + err.what());
	}
}

void BehDevice::createSurface() {
	VkSurfaceKHR rawSurface;

	if (glfwCreateWindowSurface(_instance, window->getGLFWwindow(), nullptr, &rawSurface) != VK_SUCCESS) {  // Ugly c :(
		throw std::runtime_error("Failed to create window surface!");
	}

	_surface = rawSurface;
	mainDeletionQueue.push_function([&]() { _instance.destroySurfaceKHR(_surface); });
}

void BehDevice::createLogicalDevice() {
	QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);

	std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value(),
											  indices.transferFamily.value()};

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		vk::DeviceQueueCreateInfo queueCreateInfo{
			.queueFamilyIndex = queueFamily, .queueCount = 1, .pQueuePriorities = &queuePriority};

		queueCreateInfos.push_back(queueCreateInfo);
	}

	vk::PhysicalDeviceFeatures deviceFeatures{
		.sampleRateShading = VK_TRUE,
		.fillModeNonSolid = VK_TRUE,
		.wideLines = VK_TRUE,
		.samplerAnisotropy = VK_TRUE,
	};

	vk::PhysicalDeviceImagelessFramebufferFeatures framebufferFeatures{.imagelessFramebuffer = VK_TRUE};

	vk::PhysicalDeviceDescriptorIndexingFeatures descriptorIndexFeatures{
		.pNext = &framebufferFeatures,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingPartiallyBound = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE};

	vk::DeviceCreateInfo createInfo{.pNext = &descriptorIndexFeatures,
									.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
									.pQueueCreateInfos = queueCreateInfos.data(),
									.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
									.ppEnabledExtensionNames = deviceExtensions.data(),
									.pEnabledFeatures = &deviceFeatures};

	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	} else {
		createInfo.enabledLayerCount = 0;
	}

	try {
		_device = _physicalDevice.createDevice(createInfo);
		mainDeletionQueue.push_function([&]() { _device.destroy(); });
	} catch (vk::SystemError &err) {
		throw std::runtime_error(std::string("Failed to create logical device!") + err.what());
	}

	_device.getQueue(indices.graphicsAndComputeFamily.value(), 0, &_graphicsQueue);
	_device.getQueue(indices.graphicsAndComputeFamily.value(), 0, &_computeQueue);
	_device.getQueue(indices.presentFamily.value(), 0, &_presentQueue);
	_device.getQueue(indices.transferFamily.value(), 0, &_transferQueue);
}

void BehDevice::createAllocator() {
	VmaAllocatorCreateInfo allocatorInfo{
		.flags = VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT,
		.physicalDevice = _physicalDevice,
		.device = _device,
		.instance = _instance,
		.vulkanApiVersion = apiVersion};
	vmaCreateAllocator(&allocatorInfo, &_allocator);
	mainDeletionQueue.push_function([&]() { vmaDestroyAllocator(_allocator); });
}

void BehDevice::createUploadContext() {
	QueueFamilyIndices queueFamilyIndices = findQueueFamilies(_physicalDevice, _surface);

	const vk::CommandPoolCreateInfo uploadPoolInfo{.queueFamilyIndex =
													   queueFamilyIndices.graphicsAndComputeFamily.value()};

	try {
		_uploadContext._commandPool = _device.createCommandPool(uploadPoolInfo);

		vk::CommandBufferAllocateInfo uploadAllocateInfo{
			.commandPool = _uploadContext._commandPool,
			.level = vk::CommandBufferLevel::ePrimary,
			.commandBufferCount = static_cast<uint32_t>(1),
		};

		_uploadContext._commandBuffer = _device.allocateCommandBuffers(uploadAllocateInfo)[0];

		vk::FenceCreateInfo uploadFenceInfo{};
		_uploadContext._uploadFence = _device.createFence(uploadFenceInfo);
		mainDeletionQueue.push_function([&]() {
			_device.destroyFence(_uploadContext._uploadFence);
			_device.destroyCommandPool(_uploadContext._commandPool);
		});
	} catch (vk::SystemError &err) {
		throw std::runtime_error(std::string("Failed in creating upload context") + err.what());
	}
}

std::vector<const char *> BehDevice::getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	return extensions;
}

bool BehDevice::validateExtensions(const std::vector<const char *> &extensions,
								   const std::vector<vk::ExtensionProperties> &supportedExtensions) {
	for (const auto &extension : extensions) {
		bool extensionFound = false;

		for (const auto &supportedExtension : supportedExtensions) {
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

bool BehDevice::checkValidationLayerSupport() const {
	std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

	for (const char *layerName : validationLayers) {
		bool layerFound = false;

		for (const auto &layerProperties : availableLayers) {
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

VkBool32 debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
					   VkDebugUtilsMessageTypeFlagsEXT messageType,
					   const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {
	if (strncmp(pCallbackData->pMessage, "Device Extension:", strlen("Device Extension:")) == 0) {
		return VK_TRUE;  // Don't spam with loaded layers
	}

	std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

void BehDevice::setupDebugMessenger() {
	if constexpr (!enableValidationLayers) return;

	const vk::DebugUtilsMessengerCreateInfoEXT createInfo = {
		.messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
						   vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
						   vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
		.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
					   vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
					   vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation,
		.pfnUserCallback = debugCallback};

	if (createDebugUtilsMessengerEXT(_instance,
									 reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT *>(&createInfo), nullptr,
									 &debugMessenger) != VK_SUCCESS) {
		throw std::runtime_error("failed to set up debug callback!");
	}
	mainDeletionQueue.push_function([&]() { destroyDebugUtilsMessengerEXT(_instance, debugMessenger, nullptr); });
}

VkResult BehDevice::createDebugUtilsMessengerEXT(VkInstance instance,
												 const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
												 const VkAllocationCallbacks *pAllocator,
												 VkDebugUtilsMessengerEXT *pCallback) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(instance, pCreateInfo, pAllocator, pCallback);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void BehDevice::destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback,
											  const VkAllocationCallbacks *pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, callback, pAllocator);
	}
}

void BehDevice::pickPhysicalDevice() {
	auto devices = _instance.enumeratePhysicalDevices();

	if (devices.empty()) {
		throw std::runtime_error("Failed to find GPUs with Vulkan Support!");
	}

	// Get the score of devices
	std::multimap<int, vk::PhysicalDevice> candidates;

	for (const auto &physicalDevice : devices) {
		int score = rateDeviceSuitability(physicalDevice);
		candidates.insert(std::make_pair(score, physicalDevice));
	}

	// Check if any suitable candidate was found
	if (candidates.rbegin()->first > 0) {
		_physicalDevice = candidates.rbegin()->second;
		_msaaSamples = getMaxUsableSampleCount(_physicalDevice);
	} else {
		throw std::runtime_error("Failed to find suitable GPU!");
	}
}

int BehDevice::rateDeviceSuitability(vk::PhysicalDevice physicalDevice) {
	auto deviceProperties = physicalDevice.getProperties();
	auto deviceFeatures = physicalDevice.getFeatures();

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
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, _surface);
	if (!indices.isComplete()) {
		return 0;
	}

	if (!checkDeviceExtensionSupport(physicalDevice)) {
		return 0;
	}

	// Check if the physicalDevice has a adequate swap chain
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
	if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty()) {
		return 0;
	}

	if (deviceFeatures.samplerAnisotropy == VK_FALSE) {
		return 0;
	}

	return score;
}

vk::SampleCountFlagBits BehDevice::getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice) {
	auto physicalDeviceProperties = physicalDevice.getProperties();

	auto counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
				  physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & vk::SampleCountFlagBits::e64) {
		return vk::SampleCountFlagBits::e64;
	}
	if (counts & vk::SampleCountFlagBits::e32) {
		return vk::SampleCountFlagBits::e32;
	}
	if (counts & vk::SampleCountFlagBits::e16) {
		return vk::SampleCountFlagBits::e16;
	}
	if (counts & vk::SampleCountFlagBits::e8) {
		return vk::SampleCountFlagBits::e8;
	}
	if (counts & vk::SampleCountFlagBits::e4) {
		return vk::SampleCountFlagBits::e4;
	}
	if (counts & vk::SampleCountFlagBits::e2) {
		return vk::SampleCountFlagBits::e2;
	}

	return vk::SampleCountFlagBits::e1;
}

bool BehDevice::checkDeviceExtensionSupport(const vk::PhysicalDevice physicalDevice) {
	auto availableExtensions = physicalDevice.enumerateDeviceExtensionProperties();

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
	for (const auto &extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

QueueFamilyIndices BehDevice::findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface) {
	QueueFamilyIndices indices;

	auto queueFamilies = device.getQueueFamilyProperties();

	int i = 0;
	for (const auto &queueFamily : queueFamilies) {
		// documentation of VkQueueFamilyProperties states that "Each queue family must support at least one queue".

		if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics &&
			queueFamily.queueFlags & vk::QueueFlagBits::eCompute) {
			indices.graphicsAndComputeFamily = i;
		}

		if (device.getSurfaceSupportKHR(i, surface)) {
			indices.presentFamily = i;
		}

		if (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer &&
			!(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
			indices.transferFamily = i;
		}

		if (indices.isComplete()) {
			break;
		}
		i++;
	}

	return indices;
}

SwapChainSupportDetails BehDevice::querySwapChainSupport(vk::PhysicalDevice physicalDevice) {
	SwapChainSupportDetails details;

	details.capabilities = physicalDevice.getSurfaceCapabilitiesKHR(_surface);
	details.formats = physicalDevice.getSurfaceFormatsKHR(_surface);
	details.presentModes = physicalDevice.getSurfacePresentModesKHR(_surface);

	return details;
}

SwapChainSupportDetails BehDevice::swapChainSupport() {
	return querySwapChainSupport(_physicalDevice);
}

QueueFamilyIndices BehDevice::queueFamilies() {
	return findQueueFamilies(_physicalDevice, _surface);
}
