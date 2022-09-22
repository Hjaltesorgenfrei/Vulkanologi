#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS // Version 145 at least
#include "AssetManager.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

std::shared_ptr<UploadedTexture> AssetManager::getTexture(std::string filename) {
	if (uploadedTextures.find(filename) == uploadedTextures.end()) {
		auto texture = std::make_shared<UploadedTexture>();
		createTextureImage(filename.c_str(), texture);
		createTextureImageView(texture);
		createTextureSampler(texture);
		uploadedTextures[filename] = texture;
	}
	return uploadedTextures[filename];
}

void AssetManager::createTextureImage(const char *filename, std::shared_ptr<UploadedTexture> texture) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	texture->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	vk::DeviceSize imageSize = texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image!");
	}

	std::span<stbi_uc> dataSpan{pixels, static_cast<size_t>(imageSize)};
	auto stagingBuffer = stageData(dataSpan);

	stbi_image_free(pixels);

	texture->textureImage = createImage(
			texWidth,
			texHeight,
			texture->mipLevels,
			vk::SampleCountFlagBits::e1,
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
	transitionImageLayout(
			texture->textureImage._image,
			vk::Format::eR8G8B8A8Srgb,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal,
			texture->mipLevels);
	copyBufferToImage(stagingBuffer._buffer, texture->textureImage._image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

	cleanUpStagingBuffer(stagingBuffer);

	generateMipmaps(texture->textureImage._image, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, texture->mipLevels);
	deletionQueue.push_function([&, texture]() {
		vmaDestroyImage(allocator, texture->textureImage._image, texture->textureImage._allocation);
	});
}

void AssetManager::createTextureImageView(std::shared_ptr<UploadedTexture> texture) {
	texture->textureImageView = createImageView(device, texture->textureImage._image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, texture->mipLevels);
	deletionQueue.push_function([&, texture]() {
		device.destroyImageView(texture->textureImageView);
	});
}

void AssetManager::createTextureSampler(std::shared_ptr<UploadedTexture> texture) {
	auto properties = physicalDevice.getProperties();

	vk::SamplerCreateInfo samplerInfo{
			.magFilter = vk::Filter::eLinear,
			.minFilter = vk::Filter::eLinear,
			.mipmapMode = vk::SamplerMipmapMode::eLinear,
			.addressModeU = vk::SamplerAddressMode::eRepeat,
			.addressModeV = vk::SamplerAddressMode::eRepeat,
			.addressModeW = vk::SamplerAddressMode::eRepeat,
			.mipLodBias = 0.0f,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
			.compareEnable = VK_FALSE,
			.compareOp = vk::CompareOp::eAlways,
			.minLod = 0.0f,
			.maxLod = static_cast<float>(texture->mipLevels),
			.borderColor = vk::BorderColor::eIntOpaqueBlack,
			.unnormalizedCoordinates = VK_FALSE,
	};

	if (device.createSampler(&samplerInfo, nullptr, &texture->textureSampler) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
	deletionQueue.push_function([&, texture]() {
		device.destroySampler(texture->textureSampler);
	});
}


void AssetManager::cleanUpStagingBuffer(AllocatedBuffer buffer) {
	vmaDestroyBuffer(allocator, buffer._buffer, buffer._allocation);
}

AllocatedImage AssetManager::createImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples,
										 vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags) {
	vk::ImageCreateInfo imageInfo{
			.imageType = vk::ImageType::e2D,
			.format = format,
			.extent = vk::Extent3D{
					.width = static_cast<uint32_t>(width),
					.height = static_cast<uint32_t>(height),
					.depth = 1},
			.mipLevels = mipLevels,
			.arrayLayers = 1,
			.samples = numSamples,
			.tiling = tiling,
			.usage = flags,
			.sharingMode = vk::SharingMode::eExclusive,
			.initialLayout = vk::ImageLayout::eUndefined,
	};

	AllocatedImage allocatedImage{};

	VmaAllocationCreateInfo vmaAllocCreateInfo{
			.usage = VMA_MEMORY_USAGE_GPU_ONLY};

	VkImage image;
	VkImageCreateInfo imageInfoCreate = static_cast<VkImageCreateInfo>(imageInfo);  // TODO: Add VMA HPP and fix this soup.
	if (vmaCreateImage(allocator, &imageInfoCreate, &vmaAllocCreateInfo, &image, &allocatedImage._allocation, nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image");
	}
	allocatedImage._image = image;
	return allocatedImage;
}

void AssetManager::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
										 vk::ImageLayout newLayout, uint32_t mipLevels) {
	vk::ImageMemoryBarrier barrier{
			.oldLayout = oldLayout,
			.newLayout = newLayout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = image,
			.subresourceRange = vk::ImageSubresourceRange{
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.baseMipLevel = 0,
					.levelCount = mipLevels,
					.baseArrayLayer = 0,
					.layerCount = 1}};

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	} else {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	} else {
		throw std::invalid_argument("Unsupported layout transistion!");
	}

	immediateSubmit([&](auto cmd) {
		cmd.pipelineBarrier(
				sourceStage, destinationStage,
				vk::DependencyFlags(),
				0, nullptr,
				0, nullptr,
				1, &barrier);
	});
}

void AssetManager::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
	vk::BufferImageCopy region{
			.bufferOffset = 0,
			.bufferRowLength = 0,
			.bufferImageHeight = 0,
			.imageSubresource = vk::ImageSubresourceLayers{
					.aspectMask = vk::ImageAspectFlagBits::eColor,
					.mipLevel = 0,
					.baseArrayLayer = 0,
					.layerCount = 1},
			.imageOffset = {0, 0, 0},
			.imageExtent = {width, height, 1}};

	immediateSubmit([&](auto cmd) {
		cmd.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region);
	});
}

void AssetManager::generateMipmaps(vk::Image image, vk::Format imageFormat, int32_t texWidth, int32_t texHeight,
								   uint32_t mipLevels) {
	auto properties = physicalDevice.getFormatProperties(imageFormat);

	if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
		throw std::runtime_error("Texture image format does not support linear blitting!");
	}

	immediateSubmit([&](auto commandBuffer) {
		vk::ImageMemoryBarrier barrier{
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = image,
				.subresourceRange = {
						.aspectMask = vk::ImageAspectFlagBits::eColor,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
				}};

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			commandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
					vk::DependencyFlags(),
					0, nullptr,
					0, nullptr,
					1, &barrier);

			vk::ImageBlit blit{
					.srcSubresource = {
							.aspectMask = vk::ImageAspectFlagBits::eColor,
							.mipLevel = i - 1,
							.baseArrayLayer = 0,
							.layerCount = 1},
					.dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = i, .baseArrayLayer = 0, .layerCount = 1}};
			blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
			blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
			blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
			blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};

			commandBuffer.blitImage(
					image, vk::ImageLayout::eTransferSrcOptimal,
					image, vk::ImageLayout::eTransferDstOptimal,
					1, &blit,
					vk::Filter::eLinear);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(
					vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
					vk::DependencyFlags(),
					0, nullptr,
					0, nullptr,
					1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = mipLevels - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(
				vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
				vk::DependencyFlags(),
				0, nullptr,
				0, nullptr,
				1, &barrier);
	});
}

bool AssetManager::hasStencilComponent(vk::Format format) {
	return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}

void AssetManager::immediateSubmit(std::function<void(vk::CommandBuffer)> &&function) {
	auto& commandBuffer = uploadContext._commandBuffer;
	vk::CommandBufferBeginInfo beginInfo{
			.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit};

	commandBuffer.begin(beginInfo);
	function(commandBuffer);
	commandBuffer.end();

	vk::SubmitInfo submitInfo{
			.commandBufferCount = 1,
			.pCommandBuffers = &commandBuffer};
	auto& fence = uploadContext._uploadFence;
	if (transferQueue.submit(1, &submitInfo, fence) != vk::Result::eSuccess) {
		throw std::runtime_error("Immediate submit failed!");
	}
	if (device.waitForFences(1, &fence, true, 99999999) != vk::Result::eSuccess) {
		throw std::runtime_error("Immediate submit failed!");
	}
	if (device.resetFences(1, &fence) != vk::Result::eSuccess) {
		throw std::runtime_error("Immediate submit failed!");
	}
	device.resetCommandPool(uploadContext._commandPool);
}
