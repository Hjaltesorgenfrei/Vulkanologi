#pragma once

#include <vulkan/vulkan.hpp>

// TODO: Delete this function and use assetmanager instead
vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format, vk::Flags<vk::ImageAspectFlagBits> aspectFlags, uint32_t mipLevels, uint32_t layerCount = 1);
