#pragma once

#include <map>
#include <span>
#include <vulkan/vulkan.hpp>

#include "Deletionqueue.h"
#include "BehVkInit.h"
#include "BehVkTypes.h"
#include "BehDevice.h"

class AssetManager {
   public:
    explicit AssetManager(std::shared_ptr<BehDevice>& device) : device{device} {

    }
    ~AssetManager() {
        cleanUp();
    }

    AssetManager& operator=(const AssetManager&) = delete;
    AssetManager(const AssetManager&) = delete;

    void cleanUp() {
        device->device().waitIdle();
        deletionQueue.flush();
    }

    std::shared_ptr<UploadedTexture> getTexture(const std::string& filename);

    [[nodiscard]] AllocatedImage createImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags);

    template <typename T>
    [[nodiscard]] std::vector<std::shared_ptr<AllocatedBuffer>> createBuffers(std::span<T> data, vk::BufferUsageFlags bufferUsage, size_t count = 1);

   private:
    std::shared_ptr<BehDevice> device;

    DeletionQueue deletionQueue{};
    std::map<std::string, std::shared_ptr<UploadedTexture>> uploadedTextures;

    void createTextureImage(const char* filename, const std::shared_ptr<UploadedTexture>& texture);

    void createTextureImageView(const std::shared_ptr<UploadedTexture>& texture);

    void createTextureSampler(const std::shared_ptr<UploadedTexture>& texture);

    template <typename T>
    [[nodiscard]] AllocatedBuffer stageData(std::span<T>& dataToStage);

    void cleanUpBuffer(AllocatedBuffer buffer);

    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, uint32_t mipLevels);

    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

    void generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    static bool hasStencilComponent(vk::Format format);
};

template <typename T>
inline AllocatedBuffer AssetManager::stageData(std::span<T>& dataToStage) {
    VkBufferCreateInfo stagingCreate {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = dataToStage.size() * sizeof(T),
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    };

    VmaAllocationCreateInfo stagingAlloc {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    AllocatedBuffer stagingBuffer{};

    if (vmaCreateBuffer(device->allocator(), &stagingCreate, &stagingAlloc, reinterpret_cast<VkBuffer *>(&stagingBuffer._buffer), &stagingBuffer._allocation, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create staging buffer!");
    }

    void* data;
    vmaMapMemory(device->allocator(), stagingBuffer._allocation, &data);
    {
        memcpy(data, dataToStage.data(), dataToStage.size() * sizeof(T));
    }
    vmaUnmapMemory(device->allocator(), stagingBuffer._allocation);
    return stagingBuffer;
}

template <typename T>
inline std::vector<std::shared_ptr<AllocatedBuffer>> AssetManager::createBuffers(std::span<T> data, vk::BufferUsageFlags bufferUsage, size_t count) {
    auto stagedBuffer = stageData(data);
    auto size = data.size() * sizeof(T);
    
    VkBufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = static_cast<VkBufferUsageFlags>(bufferUsage)
    };

    VmaAllocationCreateInfo allocInfo {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    std::vector<std::shared_ptr<AllocatedBuffer>> buffers;

    for (int i = 0; i < count; i++) {
        std::shared_ptr<AllocatedBuffer> buffer = std::make_shared<AllocatedBuffer>();
        if (vmaCreateBuffer(device->allocator(), &createInfo, &allocInfo, reinterpret_cast<VkBuffer*>(&buffer->_buffer), &buffer->_allocation, nullptr) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer!");
        }
        buffers.push_back(buffer);
        deletionQueue.push_function([buffer, this](){
            cleanUpBuffer(*buffer);
        });
    }

    device->immediateSubmit([&](auto cmd) {
        vk::BufferCopy copyRegion{
            .size = size
        };
        for (int i = 0; i < count; i++) {
            cmd.copyBuffer(stagedBuffer._buffer, buffers[i]->_buffer, 1, &copyRegion);
        }
    });
    cleanUpBuffer(stagedBuffer);
    return buffers;
}