#include "AssetManager.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <ktx.h>
#include <stb_image.h>

std::shared_ptr<UploadedTexture> AssetManager::getTexture(const std::string& filename) {
	if (uploadedTextures.find(filename) == uploadedTextures.end()) {
		if (filename.find("cubemap") !=
			std::string::npos) {  // TODO: Fix this nasty hack, it should just use the check in ktx.
			getCubeMap(filename);
			return uploadedTextures[filename];
		}
		auto texture = std::make_shared<UploadedTexture>();
		if (filename.ends_with(".ktx")) {
			ktxTexture2* ktx_texture;
			auto result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
														 (ktxTexture**)&ktx_texture);
			if (result != KTX_SUCCESS) {
				throw std::runtime_error("Failed to load ktx texture image!");
			}

			ktx_transcode_fmt_e target_format = KTX_TTF_BC7_RGBA;
			if (ktxTexture2_NeedsTranscoding(ktx_texture)) {
				ktxTexture2_TranscodeBasis(ktx_texture, target_format, 0);
			}
			texture->textureImageFormat = (vk::Format)ktx_texture->vkFormat;
			std::span<ktx_uint8_t> dataSpan{ktx_texture->pData, static_cast<size_t>(ktx_texture->dataSize)};

			auto stagingBuffer = stageData(dataSpan);
			texture->mipLevels = ktx_texture->numLevels;

			texture->textureImage =
				createImage(ktx_texture->baseWidth, ktx_texture->baseHeight, texture->mipLevels,
							vk::SampleCountFlagBits::e1, texture->textureImageFormat, vk::ImageTiling::eOptimal,
							vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
			texture->textureImage._layout = vk::ImageLayout::eUndefined;

			transitionImageLayout(texture, vk::ImageLayout::eTransferDstOptimal);

			copyBufferToImage(stagingBuffer._buffer, texture->textureImage._image, ktx_texture->baseWidth,
							  ktx_texture->baseHeight);

			transitionImageLayout(texture, vk::ImageLayout::eShaderReadOnlyOptimal);

			cleanUpBuffer(stagingBuffer);
			deletionQueue.push_function([&, texture]() {
				vmaDestroyImage(device->allocator(), texture->textureImage._image, texture->textureImage._allocation);
			});
		} else {
			// TODO: THIS is pretty soup the functions duplicate behavior.
			createTextureImage(filename.c_str(), texture);
		}
		createTextureImageView(texture);
		createTextureSampler(texture,
							 texture->height + texture->width < 65 ? vk::Filter::eNearest : vk::Filter::eLinear);
		uploadedTextures[filename] = texture;
	}
	return uploadedTextures[filename];
}

std::shared_ptr<UploadedTexture> AssetManager::getCubeMap(const std::string& filename) {
	if (uploadedTextures.find(filename) == uploadedTextures.end()) {
		auto texture = std::make_shared<UploadedTexture>();
		texture->layerCount = 6;
		if (filename.ends_with(".ktx")) {
			ktxTexture* ktx_texture;
			auto result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
														 (ktxTexture**)&ktx_texture);
			if (result != KTX_SUCCESS) {
				throw std::runtime_error("Failed to load ktx texture image!");
			}

			if (ktx_texture->classId == class_id::ktxTexture2_c) {
				throw std::runtime_error("Tried to load ktxTexture2 which im missing support for.");
			}

			std::span<ktx_uint8_t> dataSpan{ktx_texture->pData, static_cast<size_t>(ktx_texture->dataSize)};
			texture->textureImageFormat = vk::Format::eR8G8B8A8Unorm;  // TODO: Hardcoded right now

			auto stagingBuffer = stageData(dataSpan);
			texture->mipLevels = ktx_texture->numLevels;

			texture->textureImage =
				createCubeImage(ktx_texture->baseWidth, ktx_texture->baseHeight, texture->mipLevels,
								vk::SampleCountFlagBits::e1, texture->textureImageFormat, vk::ImageTiling::eOptimal,
								vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
			texture->textureImage._layout = vk::ImageLayout::eUndefined;

			transitionImageLayout(texture, vk::ImageLayout::eTransferDstOptimal);

			std::vector<vk::BufferImageCopy> bufferCopyRegions;

			for (uint32_t face = 0; face < 6; face++) {
				for (uint32_t level = 0; level < texture->mipLevels; level++) {
					// Calculate offset into staging buffer for the current mip level and face
					ktx_size_t offset;
					KTX_error_code ret = ktxTexture_GetImageOffset(ktx_texture, level, 0, face, &offset);
					assert(ret == KTX_SUCCESS);
					VkBufferImageCopy bufferCopyRegion = {};

					vk::BufferImageCopy region{
						.bufferOffset = offset,
						.bufferRowLength = 0,
						.bufferImageHeight = 0,
						.imageSubresource = vk::ImageSubresourceLayers{.aspectMask = vk::ImageAspectFlagBits::eColor,
																	   .mipLevel = level,
																	   .baseArrayLayer = face,
																	   .layerCount = 1},
						.imageOffset = {0, 0, 0},
						.imageExtent = {.width = ktx_texture->baseWidth >> level,
										.height = ktx_texture->baseHeight >> level,
										.depth = 1}};

					bufferCopyRegions.push_back(region);
				}
			}

			device->immediateSubmit([&](auto cmd) {
				cmd.copyBufferToImage(stagingBuffer._buffer, texture->textureImage._image,
									  vk::ImageLayout::eTransferDstOptimal,
									  static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());
			});

			transitionImageLayout(texture, vk::ImageLayout::eShaderReadOnlyOptimal);

			cleanUpBuffer(stagingBuffer);
			deletionQueue.push_function([&, texture]() {
				vmaDestroyImage(device->allocator(), texture->textureImage._image, texture->textureImage._allocation);
			});
		} else {
			throw std::runtime_error("Loading cubemap from .png or jpeg is not supported");
		}
		createTextureImageView(texture);
		createTextureSampler(texture,
							 texture->height + texture->width < 65 ? vk::Filter::eNearest : vk::Filter::eLinear);
		uploadedTextures[filename] = texture;
	}
	return uploadedTextures[filename];
}

void AssetManager::createTextureImage(const char* filename, const std::shared_ptr<UploadedTexture>& texture) {
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	texture->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
	vk::DeviceSize imageSize = texWidth * texHeight * 4;
	texture->width = texWidth;
	texture->height = texHeight;

	if (!pixels) {
		throw std::runtime_error("Failed to load texture image \"" + std::string(filename) + "\"!");
	}

	std::span<stbi_uc> dataSpan{pixels, static_cast<size_t>(imageSize)};
	auto stagingBuffer = stageData(dataSpan);

	stbi_image_free(pixels);

	texture->textureImageFormat = vk::Format::eR8G8B8A8Srgb;

	texture->textureImage = createImage(
		texWidth, texHeight, texture->mipLevels, vk::SampleCountFlagBits::e1, texture->textureImageFormat,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);

	transitionImageLayout(texture, vk::ImageLayout::eTransferDstOptimal);
	copyBufferToImage(stagingBuffer._buffer, texture->textureImage._image, static_cast<uint32_t>(texWidth),
					  static_cast<uint32_t>(texHeight));
	cleanUpBuffer(stagingBuffer);
	generateMipmaps(texture);

	deletionQueue.push_function([&, texture]() {
		vmaDestroyImage(device->allocator(), texture->textureImage._image, texture->textureImage._allocation);
	});
}

void AssetManager::createTextureImageView(const std::shared_ptr<UploadedTexture>& texture) {
	vk::ImageViewCreateInfo viewInfo{
		.image = texture->textureImage._image,
		.viewType = vk::ImageViewType::e2D,
		.format = texture->textureImageFormat,
		.subresourceRange = vk::ImageSubresourceRange{.aspectMask = vk::ImageAspectFlagBits::eColor,
													  .baseMipLevel = 0,
													  .levelCount = texture->mipLevels,
													  .baseArrayLayer = 0,
													  .layerCount = texture->layerCount}};
	if (texture->layerCount == 6) {
		viewInfo.viewType = vk::ImageViewType::eCube;
	}

	if (device->device().createImageView(&viewInfo, nullptr, &texture->textureImageView) != vk::Result::eSuccess) {
		throw std::runtime_error("failed to create image view!");
	}

	deletionQueue.push_function([&, texture]() { device->device().destroyImageView(texture->textureImageView); });
}

void AssetManager::createTextureSampler(const std::shared_ptr<UploadedTexture>& texture, vk::Filter filter) {
	auto properties = device->physicalDevice().getProperties();

	vk::SamplerCreateInfo samplerInfo{
		.magFilter = filter,
		.minFilter = filter,
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

	if (device->device().createSampler(&samplerInfo, nullptr, &texture->textureSampler) != vk::Result::eSuccess) {
		throw std::runtime_error("Failed to create texture sampler!");
	}
	deletionQueue.push_function([&, texture]() { device->device().destroySampler(texture->textureSampler); });
}

void AssetManager::cleanUpBuffer(AllocatedBuffer buffer) {
	vmaDestroyBuffer(device->allocator(), buffer._buffer, buffer._allocation);
}

AllocatedImage AssetManager::createImage(int width, int height, uint32_t mipLevels, vk::SampleCountFlagBits numSamples,
										 vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags flags) {
	vk::ImageCreateInfo imageInfo{
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent =
			vk::Extent3D{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height), .depth = 1},
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = numSamples,
		.tiling = tiling,
		.usage = flags,
		.sharingMode = vk::SharingMode::eExclusive,
		.initialLayout = vk::ImageLayout::eUndefined};

	AllocatedImage allocatedImage{};

	VmaAllocationCreateInfo vmaAllocCreateInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY};

	VkImage image;
	auto imageInfoCreate = static_cast<VkImageCreateInfo>(imageInfo);  // TODO: Add VMA HPP and fix this soup.
	if (vmaCreateImage(device->allocator(), &imageInfoCreate, &vmaAllocCreateInfo, &image, &allocatedImage._allocation,
					   nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image");
	}
	allocatedImage._image = image;
	return allocatedImage;
}

AllocatedImage AssetManager::createCubeImage(int width, int height, uint32_t mipLevels,
											 vk::SampleCountFlagBits numSamples, vk::Format format,
											 vk::ImageTiling tiling, vk::ImageUsageFlags flags) {
	vk::ImageCreateInfo imageInfo{
		.flags = vk::ImageCreateFlagBits::eCubeCompatible,
		.imageType = vk::ImageType::e2D,
		.format = format,
		.extent =
			vk::Extent3D{.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height), .depth = 1},
		.mipLevels = mipLevels,
		.arrayLayers = 6,
		.samples = numSamples,
		.tiling = tiling,
		.usage = flags,
		.sharingMode = vk::SharingMode::eExclusive,
		.initialLayout = vk::ImageLayout::eUndefined};

	AllocatedImage allocatedImage{};

	VmaAllocationCreateInfo vmaAllocCreateInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY};

	VkImage image;
	auto imageInfoCreate = static_cast<VkImageCreateInfo>(imageInfo);  // TODO: Add VMA HPP and fix this soup.
	if (vmaCreateImage(device->allocator(), &imageInfoCreate, &vmaAllocCreateInfo, &image, &allocatedImage._allocation,
					   nullptr) != VK_SUCCESS) {
		throw std::runtime_error("Failed to allocate image");
	}
	allocatedImage._image = image;
	return allocatedImage;
}

void AssetManager::transitionImageLayout(const std::shared_ptr<UploadedTexture> texture, vk::ImageLayout newLayout) {
	auto oldLayout = texture->textureImage._layout;

	vk::ImageMemoryBarrier barrier{
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = texture->textureImage._image,
		.subresourceRange = vk::ImageSubresourceRange{.aspectMask = vk::ImageAspectFlagBits::eColor,
													  .baseMipLevel = 0,
													  .levelCount = texture->mipLevels,
													  .baseArrayLayer = 0,
													  .layerCount = texture->layerCount}};

	vk::PipelineStageFlags sourceStage;
	vk::PipelineStageFlags destinationStage;

	if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

		if (hasStencilComponent(texture->textureImageFormat)) {
			barrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
		}
	} else {
		barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
	}

	if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
		barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eTransfer;
	} else if (oldLayout == vk::ImageLayout::eTransferDstOptimal &&
			   newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		sourceStage = vk::PipelineStageFlagBits::eTransfer;
		destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
	} else if (oldLayout == vk::ImageLayout::eUndefined &&
			   newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
		barrier.dstAccessMask =
			vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
	} else {
		throw std::invalid_argument("Unsupported layout transistion!");
	}

	device->immediateSubmit([&](auto cmd) {
		cmd.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
	});
	texture->textureImage._layout = newLayout;
}

void AssetManager::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
	vk::BufferImageCopy region{
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource =
			vk::ImageSubresourceLayers{
				.aspectMask = vk::ImageAspectFlagBits::eColor, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1},
		.imageOffset = {0, 0, 0},
		.imageExtent = {width, height, 1}};

	device->immediateSubmit(
		[&](auto cmd) { cmd.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &region); });
}

void AssetManager::generateMipmaps(const std::shared_ptr<UploadedTexture> texture) {
	auto properties = device->physicalDevice().getFormatProperties(texture->textureImageFormat);

	if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
		throw std::runtime_error("Texture image format does not support linear blitting!");
	}

	device->immediateSubmit([&](auto commandBuffer) {
		vk::ImageMemoryBarrier barrier{.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
									   .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
									   .image = texture->textureImage._image,
									   .subresourceRange = {
										   .aspectMask = vk::ImageAspectFlagBits::eColor,
										   .levelCount = 1,
										   .baseArrayLayer = 0,
										   .layerCount = 1,
									   }};

		int32_t mipWidth = texture->width;
		int32_t mipHeight = texture->height;

		for (uint32_t i = 1; i < texture->mipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
			barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
										  vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);

			vk::ImageBlit blit{.srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
												  .mipLevel = i - 1,
												  .baseArrayLayer = 0,
												  .layerCount = 1},
							   .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor,
												  .mipLevel = i,
												  .baseArrayLayer = 0,
												  .layerCount = 1}};
			blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
			blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
			blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
			blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};

			commandBuffer.blitImage(texture->textureImage._image, vk::ImageLayout::eTransferSrcOptimal,
									texture->textureImage._image, vk::ImageLayout::eTransferDstOptimal, 1, &blit,
									vk::Filter::eLinear);

			barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
			barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
			barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
										  vk::PipelineStageFlagBits::eFragmentShader, vk::DependencyFlags(), 0, nullptr,
										  0, nullptr, 1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = texture->mipLevels - 1;
		barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
		barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
		barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
		barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

		commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
									  vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);
	});
	texture->textureImage._layout = vk::ImageLayout::eShaderReadOnlyOptimal;
}

bool AssetManager::hasStencilComponent(vk::Format format) {
	return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
}
