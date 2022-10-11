#pragma once

#include "WindowWrapper.h"

#include <memory>

#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS  // Version 145 at least
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "AssetManager.h"
#include "Deletionqueue.h"
#include "BehVkTypes.h"
#include "BehDevice.h"
#include "BehDescriptors.h"
#include "BehPipelines.h"
#include "BehFrameInfo.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

std::vector<char> readFile(const std::string& filename);

enum RendererMode {
	NORMAL,
	WIREFRAME
};

class Renderer {
   public:
    Renderer(std::shared_ptr<WindowWrapper> window, std::shared_ptr<BehDevice> device, AssetManager &assetManager);
    ~Renderer();
    Renderer& operator=(const Renderer&) = delete;
    Renderer(const Renderer&) = delete;

    void drawFrame(FrameInfo& frameInfo);
    void uploadMeshes(const std::vector<std::shared_ptr<RenderObject>>& objects);
    void frameBufferResized();
    Material createMaterial(std::vector<std::string>& texturePaths);
	RendererMode rendererMode = RendererMode::NORMAL;

private:
    std::shared_ptr<WindowWrapper> window;
    std::shared_ptr<BehDevice> device;
    AssetManager& assetManager;

    DeletionQueue mainDeletionQueue;
    DeletionQueue swapChainDeletionQueue;

    vk::DescriptorPool imguiPool;

    vk::SwapchainKHR swapChain;
    vk::Format swapChainImageFormat;
    uint32_t swapChainImageCount;
    vk::Extent2D swapChainExtent;
    std::vector<vk::Image> swapChainImages;
    std::vector<vk::ImageView> swapChainImageViews;
    std::vector<vk::Framebuffer> swapChainFramebuffers;

    vk::DescriptorSetLayout materialDescriptorSetLayout;
    vk::DescriptorSetLayout uboDescriptorSetLayout;
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::DescriptorPool descriptorPool;
    vk::RenderPass renderPass;

    DescriptorAllocator descriptorAllocator;
    DescriptorLayoutCache descriptorLayoutCache;

    vk::PipelineLayout pipelineLayout;
    vk::PipelineLayout billboardPipelineLayout;

    std::unique_ptr<BehPipeline> graphicsPipeline;
    std::unique_ptr<BehPipeline> billboardPipeline;
    std::unique_ptr<BehPipeline> wireframePipeline;

    vk::CommandPool commandPool;
    vk::CommandPool transferCommandPool;
    std::vector<vk::CommandBuffer> commandBuffers;

    std::vector<vk::Semaphore> imageAvailableSemaphores;
    std::vector<vk::Semaphore> renderFinishedSemaphores;
    std::vector<vk::Fence> inFlightFences;
    std::vector<vk::Fence> imagesInFlight;

    std::vector<std::shared_ptr<UploadedTexture>> textures;

    AllocatedImage depthImage;
    vk::ImageView depthImageView;

    std::vector<AllocatedBuffer> uniformBuffers;

    AllocatedImage colorImage;
    vk::ImageView colorImageView;

    size_t currentFrame = 0;

    bool frameBufferResizePending = false;

    void initImgui();

    void createSwapChain();

    void recreateSwapchain();

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR>& availableFormats);

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<enum vk::PresentModeKHR>& availablePresentModes);

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    void createImageViews();

    void createRenderPass();
    void createGlobalDescriptorSetLayout();
    void createMaterialDescriptorSetLayout();

    void createGraphicsPipelineLayout();
    void createBillboardPipelineLayout();

    void createPipelines();
    void createGraphicsPipeline();
    void createBillboardPipeline();
	void createWireframePipeline();

	vk::ShaderModule createShaderModule(const std::vector<char>& code);

    void createFramebuffers();

    void createCommandPool();
    uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) const;

    void createColorResources();
    void createDepthResources();
    vk::Format findDepthFormat();
    bool hasStencilComponent(vk::Format format);
    vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);

    template <typename T>
    AllocatedBuffer uploadBuffer(std::vector<T>& meshData, VkBufferUsageFlags usage);

    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);

    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

    void createCommandBuffers();
    void recordCommandBuffer(uint32_t index, FrameInfo& frameInfo);

    void createSyncObjects();
    void updateUniformBuffer(uint32_t currentImage, FrameInfo& frameInfo);

    void cleanup();
};
