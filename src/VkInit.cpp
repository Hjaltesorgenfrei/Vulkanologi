#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Version 145 at least
#include "VkInit.h"

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format, vk::Flags<vk::ImageAspectFlagBits> aspectFlags,
				uint32_t mipLevels) {
	vk::ImageViewCreateInfo viewInfo{
			.image = image,
			.viewType = vk::ImageViewType::e2D,
			.format = format,
			.subresourceRange = vk::ImageSubresourceRange{
					.aspectMask = aspectFlags,
					.baseMipLevel = 0,
					.levelCount = mipLevels,
					.baseArrayLayer = 0,
					.layerCount = 1}};

	vk::ImageView imageView;
	if (device.createImageView(&viewInfo, nullptr, &imageView) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create image view!");
	}

	return imageView;
}
