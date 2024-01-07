#pragma once

#include <vulkan/vulkan.hpp>

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format, vk::Flags<vk::ImageAspectFlagBits> aspectFlags, uint32_t mipLevels, uint32_t layerCount = 1);
