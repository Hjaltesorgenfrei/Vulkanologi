#pragma once
#define GLFW_INCLUDE_VULKAN
#include <memory>
#include <GLFW/glfw3.h>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Version 145 at least
#include <vulkan/vulkan.hpp>

#include <optional>
#include <string>
#include <vector>
#include <functional>

#include "Window.h"
#include "RenderData.h"
#include "VkTypes.h"


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
	std::optional<uint32_t> transferFamily;

	[[nodiscard]] bool isComplete() const {
		return graphicsFamily.has_value() && presentFamily.has_value() && transferFamily.has_value();
	}
};

struct SwapChainSupportDetails {
	vk::SurfaceCapabilitiesKHR capabilities;
	std::vector<vk::SurfaceFormatKHR> formats;
	std::vector<vk::PresentModeKHR> presentModes;
};

struct UploadContext {
    vk::Fence _uploadFence;
    vk::CommandPool _commandPool;
    vk::CommandBuffer _commandBuffer;
};

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);

std::vector<char> readFile(const std::string& filename);

class Renderer {
public:
	std::shared_ptr<WindowWrapper> window;
	std::shared_ptr<RenderData> renderData;
	
	vk::Instance instance;
	VkDebugUtilsMessengerEXT debugMessenger;
	vk::SurfaceKHR surface;

	vk::PhysicalDevice physicalDevice;
	vk::Device device;

	VmaAllocator allocator;

	vk::DescriptorPool imguiPool;

	vk::Queue presentQueue;
	vk::Queue graphicsQueue;
	vk::Queue transferQueue;

	vk::SwapchainKHR swapChain;
	vk::Format swapChainImageFormat;
	vk::Extent2D swapChainExtent;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::ImageView> swapChainImageViews;
	std::vector<vk::Framebuffer> swapChainFramebuffers;

	vk::DescriptorSetLayout descriptorSeyLayout;
	std::vector<vk::DescriptorSet> descriptorSets;
	vk::DescriptorPool descriptorPool;
	vk::PipelineLayout pipelineLayout;
	vk::RenderPass renderPass;
	vk::Pipeline graphicsPipeline;

	vk::CommandPool commandPool;
	vk::CommandPool transferCommandPool;
	std::vector<vk::CommandBuffer> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;

	uint32_t mipLevels;
    vk::Image textureImage;
    vk::DeviceMemory textureImageMemory;
	vk::ImageView textureImageView;
	vk::Sampler textureSampler;

	vk::Image depthImage;
	vk::DeviceMemory depthImageMemory;
	vk::ImageView depthImageView;

	std::vector<AllocatedBuffer> uniformBuffers;

	vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
	vk::Image colorImage;
	vk::DeviceMemory colorImageMemory;
	vk::ImageView colorImageView;

    UploadContext _uploadContext;

	size_t currentFrame = 0;

	bool framebufferResized = false;
	uint32_t graphicsQueueIndex;
	uint32_t transferQueueIndex;


	
	Renderer(std::shared_ptr<WindowWrapper>& window, std::shared_ptr<RenderData>& model);
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

	void DestroyDebugUtilsMessengerEXT(vk::Instance instance, vk::DebugUtilsMessengerEXT debugMessenger,
	                                   const vk::AllocationCallbacks* pAllocator);

	void createSurface();

	void pickPhysicalDevice();

	int rateDeviceSuitability(vk::PhysicalDevice device);

	bool checkDeviceExtensionSupport(vk::PhysicalDevice device);

	SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

	void createLogicalDevice();

	void createAllocator();

	void initImgui();

	void createSwapChain();

	void cleanupSwapChain();

	void recreateSwapchain();

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR>& availableFormats);

	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<enum vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	void createImageViews();
	vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags, uint32_t mipLevels);
	void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

	void createRenderPass();
	void createDescriptorSetLayout();

	void createGraphicsPipeline();

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

	void createFramebuffers();

	void createCommandPool();
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

	void createColorResources();
	void createDepthResources();
	vk::Format findDepthFormat();
	bool hasStencilComponent(vk::Format format);
	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

	void uploadMeshes();

    template<typename T>
    AllocatedBuffer uploadBuffer(std::vector<T>& meshData, VkBufferUsageFlags usage);
	void createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties,
	                  vk::Buffer& buffer, vk::DeviceMemory& bufferMemory);

    void createTextureImage();
    void createImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags,
                     vk::Image& image, vk::DeviceMemory& memory);
	void createTextureImageView();
	void createTextureSampler();

    void createUploadContext();
    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);
	void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

   
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

	void createCommandBuffers();
	void recordCommandBuffer(int index);

	void createSyncObjects();
	void updateUniformBuffer(uint32_t currentImage);

	void drawFrame();

	void cleanup();
};
