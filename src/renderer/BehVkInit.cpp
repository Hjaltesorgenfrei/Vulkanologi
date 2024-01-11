#include "BehVkInit.hpp"

vk::ImageView createImageView(vk::Device device, vk::Image image, vk::Format format,
							  vk::Flags<vk::ImageAspectFlagBits> aspectFlags, uint32_t mipLevels, uint32_t layerCount) {
	vk::ImageViewCreateInfo viewInfo{.image = image,
									 .viewType = vk::ImageViewType::e2D,
									 .format = format,
									 .subresourceRange = vk::ImageSubresourceRange{.aspectMask = aspectFlags,
																				   .baseMipLevel = 0,
																				   .levelCount = mipLevels,
																				   .baseArrayLayer = 0,
																				   .layerCount = layerCount}};

	vk::ImageView imageView;
	if (device.createImageView(&viewInfo, nullptr, &imageView) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create image view!");
	}

	return imageView;
}
