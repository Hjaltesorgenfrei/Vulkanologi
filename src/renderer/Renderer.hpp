#pragma once

#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "AssetManager.hpp"
#include "BehDescriptors.hpp"
#include "BehDevice.hpp"
#include "BehFrameInfo.hpp"
#include "BehPipelines.hpp"
#include "BehVkTypes.hpp"
#include "Deletionqueue.hpp"
#include "WindowWrapper.hpp"
#include "MeshHandle.hpp"
#include "Mesh.hpp"

const int MAX_FRAMES_IN_FLIGHT = 2;

std::vector<char> readFile(const std::string& filename);

enum RendererMode { NORMAL, WIREFRAME };

struct GlobalUbo;

class Renderer {
public:
	Renderer(std::shared_ptr<WindowWrapper> window, std::shared_ptr<BehDevice> device, AssetManager& assetManager);
	~Renderer();
	Renderer& operator=(const Renderer&) = delete;
	Renderer(const Renderer&) = delete;

	int drawFrame(FrameInfo& frameInfo);
	MeshHandle uploadMesh(std::string path);
	Material uploadMaterial(std::string path);
	Material createMaterial(std::vector<std::string>& texturePaths);
	void recreateSwapchain();
	uint64_t getMemoryUsage();
	RendererMode rendererMode = RendererMode::NORMAL;
	bool shouldDrawComputeParticles = false;

private:
	std::shared_ptr<WindowWrapper> window;
	std::shared_ptr<BehDevice> device;
	AssetManager& assetManager;

	DeletionQueue mainDeletionQueue;

	// Using a barrier to check for usage might be better
	DeletionQueue deleteLaterQueue;
	std::vector<DeletionQueue> oldDeleteLaterQueues;
	uint32_t awaitingSwapchainImageUsed;
	bool awaitingClean = false;

	vk::DescriptorPool imguiPool;

	vk::SwapchainKHR swapChain;
	vk::SwapchainKHR oldSwapChain{VK_NULL_HANDLE};
	vk::Format swapChainImageFormat;
	vk::Format depthFormat;
	uint32_t swapChainImageCount;
	vk::Extent2D swapChainExtent;
	std::vector<vk::Image> swapChainImages;
	std::vector<vk::ImageView> swapChainImageViews;
	vk::Framebuffer swapChainFramebuffer;
	vk::ImageUsageFlags swapChainSupportedFlags;

	vk::DescriptorSetLayout materialDescriptorSetLayout;
	vk::DescriptorSetLayout uboDescriptorSetLayout;
	vk::DescriptorSetLayout computeDescriptorSetLayout;
	std::vector<vk::DescriptorSet> descriptorSets;
	vk::DescriptorPool descriptorPool;
	vk::RenderPass renderPass;

	DescriptorAllocator descriptorAllocator;
	DescriptorLayoutCache descriptorLayoutCache;

	vk::PipelineLayout graphicsPipelineLayout;
	vk::PipelineLayout billboardPipelineLayout;
	vk::PipelineLayout computePipelineLayout;
	vk::PipelineLayout skyboxPipelineLayout;

	std::unique_ptr<BehPipeline> graphicsPipeline;
	std::unique_ptr<BehPipeline> billboardPipeline;
	std::unique_ptr<BehPipeline> skyboxPipeline;
	std::unique_ptr<BehPipeline> wireframePipeline;
	std::unique_ptr<BehPipeline> particlePipeline;
	std::unique_ptr<BehPipeline> linePipeline;
	std::unique_ptr<BehPipeline> computePipeline;

	vk::CommandPool commandPool;
	vk::CommandPool transferCommandPool;
	std::vector<vk::CommandBuffer> commandBuffers;
	std::vector<vk::CommandBuffer> computeCommandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	std::vector<vk::Fence> imagesInFlight;
	std::vector<vk::Fence> computeInFlightFences;
	std::vector<vk::Semaphore> computeFinishedSemaphores;

	std::vector<std::shared_ptr<UploadedTexture>> textures;

	AllocatedImage depthImage;
	vk::ImageView depthImageView;

	std::vector<std::shared_ptr<PersistentlyMappedBuffer<GlobalUbo>>> uniformBuffers;

	std::shared_ptr<PersistentlyMappedBuffer<Point>> lineVertexBuffer;
	std::shared_ptr<PersistentlyMappedBuffer<uint32_t>> lineIndexBuffer;

	AllocatedImage colorImage;
	vk::ImageView colorImageView;

	std::vector<std::shared_ptr<AllocatedBuffer>> shaderStorageBuffers;
	std::vector<vk::DescriptorSet> computeDescriptorSets;

	Material skyBox;
	bool displaySkybox = true;

	std::shared_ptr<Mesh> cubeMesh;

	std::unordered_map<MeshHandle, std::shared_ptr<Mesh>> meshMap;

	size_t currentFrame = 0;

	void initImgui();

	void createSwapChain();

	vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR>& availableFormats);

	vk::PresentModeKHR chooseSwapPresentMode(const std::vector<enum vk::PresentModeKHR>& availablePresentModes);

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

	void createImageViews();

	void createRenderPass();
	void createGlobalDescriptorSetLayout();
	void createMaterialDescriptorSetLayout();
	void createComputeDescriptorSetLayout();

	void createPipelineLayout(vk::PipelineLayout& pipelineLayout,
							  std::vector<vk::DescriptorSetLayout> descriptorSetLayouts,
							  std::vector<vk::PushConstantRange> pushConstantranges);

	void createPipelines();
	void createSkyboxPipeline();
	void createGraphicsPipeline();
	void createBillboardPipeline();
	void createParticlePipeline();
	void createWireframePipeline();
	void createLinePipeline();
	void createComputePipeline();

	void createComputeShaderBuffers();

	void createFramebuffers();

	void createCommandPool();
	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

	void createColorResources();
	void createDepthResources();
	vk::Format findDepthFormat();
	bool hasStencilComponent(vk::Format format);
	vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling,
								   vk::FormatFeatureFlags features);

	void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
							   uint32_t mipLevels);

	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createComputeDescriptorSets();
	void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

	void createCommandBuffers();
	void recordCommandBuffer(vk::CommandBuffer& commandBuffer, size_t index, FrameInfo& frameInfo);
	void recordComputeCommandBuffer(vk::CommandBuffer& commandBuffer, FrameInfo& frameInfo);

	void createSyncObjects();
	void updateUniformBuffer(size_t currentImage, FrameInfo& frameInfo);

	void cleanup();
};
