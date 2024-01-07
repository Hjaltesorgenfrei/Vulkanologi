#pragma once

#include <map>
#include <span>
#include <vulkan/vulkan.hpp>

#include "Deletionqueue.hpp"
#include "BehVkInit.hpp"
#include "BehVkTypes.hpp"
#include "BehDevice.hpp"

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

    std::shared_ptr<UploadedTexture> getCubeMap(const std::string& filename);

    //TODO: This should probably be private.
    [[nodiscard]] AllocatedImage createImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags);

    template <typename T>
    [[nodiscard]] std::vector<std::shared_ptr<AllocatedBuffer>> createBuffers(std::span<T> data, vk::BufferUsageFlags bufferUsage, size_t count = 1);

    template <typename T>
    [[nodiscard]] std::shared_ptr<AllocatedBuffer> createBuffer(std::span<T> data, vk::BufferUsageFlags bufferUsage) 
    {
        return createBuffers<T>(data, bufferUsage, 1)[0];
    }

    template <typename T>
    [[nodiscard]] std::shared_ptr<PersistentBuffer<T>> allocatePersistentBuffer(size_t count, vk::BufferUsageFlags bufferUsage);

   private:
    std::shared_ptr<BehDevice> device;

    DeletionQueue deletionQueue{};
    std::map<std::string, std::shared_ptr<UploadedTexture>> uploadedTextures;

    [[nodiscard]] AllocatedImage createCubeImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags);

    void createTextureImage(const char* filename, const std::shared_ptr<UploadedTexture>& texture);

    void createTextureImageView(const std::shared_ptr<UploadedTexture>& texture);

    void createTextureSampler(const std::shared_ptr<UploadedTexture>& texture, vk::Filter filter = vk::Filter::eLinear);

    template <typename T>
    [[nodiscard]] AllocatedBuffer stageData(std::span<T>& dataToStage);

    void cleanUpBuffer(AllocatedBuffer buffer);

    void transitionImageLayout(const std::shared_ptr<UploadedTexture> texture, vk::ImageLayout newLayout);

    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);

    void generateMipmaps(const std::shared_ptr<UploadedTexture> texture);

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
        .usage = static_cast<VkBufferUsageFlags>(bufferUsage | vk::BufferUsageFlagBits::eTransferDst)
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

template <typename T>
inline std::shared_ptr<PersistentBuffer<T>> AssetManager::allocatePersistentBuffer(size_t count, vk::BufferUsageFlags bufferUsage) {

    VkBufferCreateInfo createInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = count * sizeof(T),
        .usage = static_cast<VkBufferUsageFlags>(bufferUsage | vk::BufferUsageFlagBits::eTransferDst)
    };

    VmaAllocationCreateInfo allocCreateInfo {
        .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    VmaAllocationInfo allocInfo;

    std::shared_ptr<PersistentBuffer<T>> buffer = std::make_shared<PersistentBuffer<T>>();
    if (vmaCreateBuffer(device->allocator(), &createInfo, &allocCreateInfo, reinterpret_cast<VkBuffer*>(&buffer->_buffer), &buffer->_allocation, &allocInfo) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer!");
    }
    deletionQueue.push_function([buffer, this](){
        cleanUpBuffer(*buffer);
    });

    buffer->_data = reinterpret_cast<T*>(allocInfo.pMappedData);
    buffer->_count = count;
    
    return buffer;
}

