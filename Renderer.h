#pragma once
#define GLFW_INCLUDE_VULKAN
#include <memory>
#include <GLFW/glfw3.h>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Version 145 at least
#include <vulkan/vulkan.hpp>

#include <optional>
#include <string>
#include <vector>


#include "Window.h"



const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	[[nodiscard]] bool isComplete() const {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);

static std::vector<char> readFile(const std::string& filename);

class Renderer {
public:

	vk::UniqueInstance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	vk::SurfaceKHR surface;

	vk::PhysicalDevice physicalDevice;
	vk::UniqueDevice device;

	vk::Queue presentQueue;
	vk::Queue graphicsQueue;

	vk::SwapchainKHR swapChain;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::ImageView> swapChainImageViews;
	std::vector<vk::Framebuffer> swapChainFramebuffers;

	vk::PipelineLayout pipelineLayout;
	vk::RenderPass renderPass;
	vk::Pipeline graphicsPipeline;

	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;
	size_t currentFrame = 0;

	Renderer(std::unique_ptr<Window> & window);
	~Renderer();
	Renderer& operator=(const Renderer&) = delete;
	Renderer(const Renderer&) = delete;

	void createInstance();

	std::vector<const char*> getRequiredExtensions();

	bool validateExtensions(std::vector<const char*> extensions,
	                        std::vector<vk::ExtensionProperties> supportedExtensions);

	bool checkValidationLayerSupport() const;

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	void setupDebugMessenger();

	void DestroyDebugUtilsMessengerEXT(vk::UniqueInstance instance, vk::DebugUtilsMessengerEXT debugMessenger,
	                                   const vk::AllocationCallbacks* pAllocator);

	void createSurface(std::unique_ptr<Window>& window);

	void pickPhysicalDevice();

	int rateDeviceSuitability(vk::PhysicalDevice device);

	bool checkDeviceExtensionSupport(vk::PhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

	void createLogicalDevice();

	void createSwapChain(std::unique_ptr<Window>& window);

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR>& availableFormats);

	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<enum vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, std::unique_ptr<Window>& window);

	void createImageViews();

	void createRenderPass();

	void createGraphicsPipeline();

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void createFramebuffers();

	void createCommandPool();

	void createCommandBuffers();

	void createSyncObjects();

	void drawFrame();

	void cleanup();

};
