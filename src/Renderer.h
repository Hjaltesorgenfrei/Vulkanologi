#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

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
#include "DescriptorSetManager.h"
#include "RenderData.h"
#include "VkTypes.h"
#include "WindowWrapper.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

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

QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);

std::vector<char> readFile(const std::string& filename);

class Renderer {
   public:
    Renderer(std::shared_ptr<WindowWrapper>& window, std::shared_ptr<RenderData>& model);
    ~Renderer();
    void drawFrame();
    void frameBufferResized();
    Material createMaterial(std::vector<std::string>& texturePaths);

   private:
    std::shared_ptr<WindowWrapper> window;
    std::shared_ptr<RenderData> renderData;
    DeletionQueue mainDeletionQueue;
    DeletionQueue swapChainDeletionQueue;
    AssetManager assetManager;

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

    vk::DescriptorSetLayout textureDescriptorSetLayout;
    vk::DescriptorSetLayout uboDescriptorSetLayout;
    std::vector<vk::DescriptorSet> descriptorSets;
    vk::DescriptorPool descriptorPool;
    vk::RenderPass renderPass;

    vk::PipelineLayout pipelineLayout;
    vk::Pipeline graphicsPipeline;

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

    vk::SampleCountFlagBits msaaSamples = vk::SampleCountFlagBits::e1;
    AllocatedImage colorImage;
    vk::ImageView colorImageView;

    UploadContext _uploadContext;

    size_t currentFrame = 0;

    bool frameBufferResizePending = false;
    uint32_t graphicsQueueIndex;
    uint32_t transferQueueIndex;

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

    void createSurface();

    void pickPhysicalDevice();

    int rateDeviceSuitability(vk::PhysicalDevice device);

    bool checkDeviceExtensionSupport(vk::PhysicalDevice device);

    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice device);

    void createLogicalDevice();

    void createAllocator();

    void initImgui();

    void createSwapChain();

    void recreateSwapchain();

    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<struct vk::SurfaceFormatKHR>& availableFormats);

    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<enum vk::PresentModeKHR>& availablePresentModes);

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

    void createImageViews();

    void createRenderPass();
    void createDescriptorSetLayout();

    void createGraphicsPipelineLayout();
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

    template <typename T>
    AllocatedBuffer uploadBuffer(std::vector<T>& meshData, VkBufferUsageFlags usage);

    void createTextureDescriptorSetLayout();
    vk::DescriptorSet createTextureDescriptorSet(std::vector<std::shared_ptr<UploadedTexture>>& texture);

    void createUploadContext();

    void immediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function);

    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);

    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);

    void createCommandBuffers();
    void recordCommandBuffer(int index);

    void createSyncObjects();
    void updateUniformBuffer(uint32_t currentImage);

    void cleanup();
};
