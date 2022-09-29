#pragma once

#include "WindowWrapper.h"
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS  // Version 145 at least
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include "VkTypes.h"
#include "Deletionqueue.h"

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

class VulkanDevice {
public:
    explicit VulkanDevice(std::shared_ptr<WindowWrapper> window);
    ~VulkanDevice();

    VulkanDevice(const VulkanDevice &) = delete;
    VulkanDevice &operator=(const VulkanDevice &) = delete;
    VulkanDevice(VulkanDevice &&) = delete;
    VulkanDevice &operator=(VulkanDevice &&) = delete;

    void immediateSubmit(std::function<void(vk::CommandBuffer)> &&function);
    QueueFamilyIndices queueFamilies();
    SwapChainSupportDetails swapChainSupport();

    vk::SurfaceKHR surface() { return _surface; }
    vk::Device device() { return _device; }
    vk::PhysicalDevice physicalDevice() { return _physicalDevice; }
    vk::Instance instance() { return _instance; }
    vk::Queue graphicsQueue() {return _graphicsQueue; }
    vk::Queue presentQueue() {return _presentQueue; }
    vk::SampleCountFlagBits msaaSamples() { return _msaaSamples; }
    VmaAllocator allocator() { return _allocator; }

private:
    void createInstance();

    void createSurface();

    void createLogicalDevice();

    void createAllocator();

    void createUploadContext();

    static std::vector<const char*> getRequiredExtensions();

    static bool validateExtensions(const std::vector<const char*>& extensions,
                                      std::vector<vk::ExtensionProperties> supportedExtensions);

    [[nodiscard]] bool checkValidationLayerSupport() const;

    void setupDebugMessenger();

    static VkResult createDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pCallback);

    static void destroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT callback, const VkAllocationCallbacks* pAllocator);

    void pickPhysicalDevice();

    int rateDeviceSuitability(vk::PhysicalDevice physicalDevice);

    static vk::SampleCountFlagBits getMaxUsableSampleCount(vk::PhysicalDevice physicalDevice);

    bool checkDeviceExtensionSupport(vk::PhysicalDevice physicalDevice);

    static QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device, vk::SurfaceKHR surface);

    SwapChainSupportDetails querySwapChainSupport(vk::PhysicalDevice physicalDevice);

    vk::Instance _instance;
    vk::SurfaceKHR _surface;
    vk::PhysicalDevice _physicalDevice;
    vk::Device _device;

    VkDebugUtilsMessengerEXT debugMessenger{};

    vk::Queue _presentQueue;
    vk::Queue _graphicsQueue;

    vk::Queue _transferQueue;
    UploadContext _uploadContext;


    vk::SampleCountFlagBits _msaaSamples = vk::SampleCountFlagBits::e1;

    VmaAllocator _allocator{};
    DeletionQueue mainDeletionQueue;

    std::shared_ptr<WindowWrapper> window;

    const std::vector<const char*> validationLayers = {
            "VK_LAYER_KHRONOS_validation"};

    const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME};
};
