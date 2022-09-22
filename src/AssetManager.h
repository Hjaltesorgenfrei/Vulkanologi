#pragma once

#include <map>
#include <span>
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS  // Version 145 at least

#include <vulkan/vulkan.hpp>

#include "Deletionqueue.h"
#include "VkInit.h"
#include "VkTypes.h"

class AssetManager {
   public:
    void init(vk::PhysicalDevice physicalDevice,
              vk::Device device,
              vk::Queue transferQueue,
              VmaAllocator allocator,
              UploadContext uploadContext) {
        this->physicalDevice = physicalDevice;
        this->device = device;
        this->transferQueue = transferQueue;
        this->allocator = allocator;
        this->uploadContext = uploadContext;
    }

    AssetManager() {
    }
    ~AssetManager() {
        cleanUp();
    }

    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(const AssetManager&) = delete;

    void cleanUp() {
        deletionQueue.flush();
    }

    std::shared_ptr<UploadedTexture> getTexture(std::string filename);

    [[nodiscard]] AllocatedImage createImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags);

   private:
    vk::PhysicalDevice physicalDevice;
    vk::Device device;
    vk::Queue transferQueue;
    VmaAllocator allocator;
    UploadContext uploadContext;

    DeletionQueue deletionQueue{};
    std::map<std::string, std::shared_ptr<UploadedTexture>> uploadedTextures;

    void createTextureImage(const char* filename, std::shared_ptr<UploadedTexture> texture);

    void createTextureImageView(std::shared_ptr<UploadedTexture> texture);

    void createTextureSampler(std::shared_ptr<UploadedTexture> texture);

    template <typename T>
    [[nodiscard]] AllocatedBuffer stageData(std::span<T>& dataToStage);

    void cleanUpStagingBuffer(AllocatedBuffer buffer);

    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);

    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

    void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    bool hasStencilComponent(vk::Format format);

    void immediateSubmit(std::function<void(vk::CommandBuffer cmd)>&& function);
};

template <typename T>
inline AllocatedBuffer AssetManager::stageData(std::span<T>& dataToStage) {
    VkBufferCreateInfo stagingCreate{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = dataToStage.size() * sizeof(T),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT};

    VmaAllocationCreateInfo stagingAlloc{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO};

    VkBuffer buffer;
    AllocatedBuffer stagingBuffer{};

    if (vmaCreateBuffer(allocator, &stagingCreate, &stagingAlloc, &buffer, &stagingBuffer._allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to uploadBuffer mesh vertices!");
    }
    stagingBuffer._buffer = buffer;

    void* data;
    vmaMapMemory(allocator, stagingBuffer._allocation, &data);
    {
        memcpy(data, dataToStage.data(), dataToStage.size() * sizeof(T));
    }
    vmaUnmapMemory(allocator, stagingBuffer._allocation);
    return stagingBuffer;
}