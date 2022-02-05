#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

struct AllocatedBuffer {
    vk::Buffer _buffer; 
    VmaAllocation _allocation;
};

struct AllocatedImage {
    vk::Image _image;
    VmaAllocation _allocation;
};