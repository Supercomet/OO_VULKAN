/************************************************************************************//*!
\file           VulkanTexture.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Defines the texture wrapper object

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "VulkanTexture.h"
#include "DelayedDeleter.h"

#pragma warning( push )
#pragma warning( disable : 26451 ) // arithmetic overflow
#include "loader/stb_image.h"
#pragma warning( pop )

#include <cassert>

namespace vkutils
{
	void Texture::updateDescriptor()
	{
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;
	}

	void Texture::destroy(bool delayed)
	{
		auto viewCpy = view;
		auto imageCpy = image;
		auto memoryCpy = deviceMemory;
		auto deviceCpy = device->logicalDevice;

		// deletion func
		auto delFunctor = [=](){
			vkDestroyImageView(deviceCpy, viewCpy, nullptr);

			vkDestroyImage(deviceCpy, imageCpy, nullptr);			
			vkFreeMemory(deviceCpy, memoryCpy, nullptr);
		};

		if (delayed)
		{
			auto* dd = DelayedDeleter::get();
			dd->DeleteAfterFrames(delFunctor);
		}
		else
		{
			delFunctor();
		}
		
		view = VK_NULL_HANDLE;		
		image = VK_NULL_HANDLE;
		deviceMemory = VK_NULL_HANDLE;
	}

	/**
	* Load a 2D texture including all mip levels
	*
	* @param filename File to load (supports .ktx)
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	* @param (Optional) forceLinear Force linear tiling (not advised, defaults to false)
	*
	*/
	void Texture2D::loadFromFile(std::string filename, VkFormat _format, VulkanDevice *device, VkQueue copyQueue, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout, bool forceLinear)
	{
		stbi_uc* ktxTextureData;
		int _width, _height, _channels;
		ktxTextureData = stbi_load(filename.c_str(),&_width,&_height, &_channels, STBI_rgb_alpha);

		this->device = device;
		width = _width;
		height = _height;
		format = _format;
		usage = imageUsageFlags;
		int ktxTextureSize = (width * height * STBI_rgb_alpha);

		//TODO mips
		mipLevels = 0;

		// Get device properties for the requested texture _format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(device->physicalDevice, _format, &formatProperties);

		// Only use linear tiling if requested (and supported by the device)
		// Support for linear tiling is mostly limited, so prefer to use
		// optimal tiling instead
		// On most implementations linear tiling will only support a very
		// limited amount of _formats and features (mip maps, cubemaps, arrays, etc.)
		VkBool32 useStaging = !forceLinear;

		VkMemoryAllocateInfo memAllocInfo =oGFX::vkutils::inits::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, device->commandPoolManagers[0].m_commandpool, true);

		if (useStaging)
		{
			// Create a host-visible staging buffer that contains the raw image data
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			VkBufferCreateInfo bufferCreateInfo = oGFX::vkutils::inits::bufferCreateInfo();
			bufferCreateInfo.size = ktxTextureSize;
			// This buffer is used as a transfer source for the buffer copy
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

			// Get memory requirements for the staging buffer (alignment, memory type bits)
			vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;
			// Get memory type index for a host visible buffer
			memAllocInfo.memoryTypeIndex = oGFX::FindMemoryTypeIndex(device->physicalDevice,memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			
			vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory);
			vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0);

			// Copy texture data into staging buffer
			uint8_t *data;
			vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data);
			memcpy(data, ktxTextureData, ktxTextureSize);
			vkUnmapMemory(device->logicalDevice, stagingMemory);

			// Setup buffer copy regions for each mip level
			//std::vector<VkBufferImageCopy> bufferCopyRegions;
			//
			//for (uint32_t i = 0; i < mipLevels; i++)
			//{
			//	ktx_size_t offset;
			//	KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
			//	assert(result == KTX_SUCCESS);
			//
			//	VkBufferImageCopy bufferCopyRegion = {};
			//	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			//	bufferCopyRegion.imageSubresource.mipLevel = i;
			//	bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			//	bufferCopyRegion.imageSubresource.layerCount = 1;
			//	bufferCopyRegion.imageExtent.width = std::max(1u, ktxTexture->baseWidth >> i);
			//	bufferCopyRegion.imageExtent.height = std::max(1u, ktxTexture->baseHeight >> i);
			//	bufferCopyRegion.imageExtent.depth = 1;
			//	bufferCopyRegion.bufferOffset = offset;
			//
			//	bufferCopyRegions.push_back(bufferCopyRegion);
			//}

			// Create optimal tiled target image
			VkImageCreateInfo imageCreateInfo = oGFX::vkutils::inits::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = _format;
			imageCreateInfo.mipLevels = mipLevels;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.usage = imageUsageFlags;
			// Ensure that the TRANSFER_DST bit is set for staging
			//if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
			{
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
			}
			vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image);
			VK_NAME(device->logicalDevice, "loadFromFile::image", image);

			vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;

			memAllocInfo.memoryTypeIndex = oGFX::FindMemoryTypeIndex(device->physicalDevice,memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
			vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory);
			vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0);

			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mipLevels;
			subresourceRange.layerCount = 1;

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy
			oGFX::vkutils::tools::setImageLayout(
				copyCmd,
				image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			//// Copy mip levels from staging buffer
			//vkCmdCopyBufferToImage(
			//	copyCmd,
			//	stagingBuffer,
			//	image,
			//	VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			//	static_cast<uint32_t>(bufferCopyRegions.size()),
			//	bufferCopyRegions.data()
			//);

			// Change texture image layout to shader read after all mip levels have been copied
			this->imageLayout = imageLayout;
			oGFX::vkutils::tools::setImageLayout(
				copyCmd,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				imageLayout,
				subresourceRange);

			device->FlushCommandBuffer(copyCmd, copyQueue, device->commandPoolManagers[0].m_commandpool);

			// Clean up staging resources
			vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
			vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
		}
		else
		{
			// Prefer using optimal tiling, as linear tiling 
			// may support only a small set of features 
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			// Check if this support is supported for linear tiling
			assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			VkImageCreateInfo imageCreateInfo = oGFX::vkutils::inits::imageCreateInfo();
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = _format;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = imageUsageFlags;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			// Load mip map level 0 to linear tiling image
			vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &mappableImage);
			VK_NAME(device->logicalDevice, "loadFromFile::mappableImage", mappableImage);

			// Get memory requirements for this image 
			// like size and alignment
			vkGetImageMemoryRequirements(device->logicalDevice, mappableImage, &memReqs);
			// Set memory allocation size to required memory size
			memAllocInfo.allocationSize = memReqs.size;

			// Get memory type that can be mapped to host memory
			memAllocInfo.memoryTypeIndex = oGFX::FindMemoryTypeIndex(device->physicalDevice	,memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			
			// Allocate host memory
			vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &mappableMemory);

			// Bind allocated image for use
			vkBindImageMemory(device->logicalDevice, mappableImage, mappableMemory, 0);

			// Get sub resource layout
			// Mip map count, array layer, etc.
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subRes.mipLevel = 0;

			VkSubresourceLayout subResLayout;
			void *data;

			// Get sub resources layout 
			// Includes row pitch, size offsets, etc.
			vkGetImageSubresourceLayout(device->logicalDevice, mappableImage, &subRes, &subResLayout);

			// Map image memory
			vkMapMemory(device->logicalDevice, mappableMemory, 0, memReqs.size, 0, &data);

			// Copy image data into memory
			memcpy(data, ktxTextureData, memReqs.size);

			vkUnmapMemory(device->logicalDevice, mappableMemory);

			// Linear tiled images don't need to be staged
			// and can be directly used as textures
			image = mappableImage;
			deviceMemory = mappableMemory;
			this->imageLayout = imageLayout;

			// Setup image memory barrier
			oGFX::vkutils::tools::setImageLayout(copyCmd, image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

			device->FlushCommandBuffer(copyCmd, copyQueue,device->commandPoolManagers[0].m_commandpool);
		}

		stbi_image_free(ktxTextureData);

		filter = VK_FILTER_LINEAR;

		

		// Create image view
		// Textures are not directly accessed by the shaders and
		// are abstracted by image views containing additional
		// in_formation and sub resource ranges
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = _format;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		// Linear tiling usually won't support mip maps
		// Only set mip map count if optimal tiling is used
		viewCreateInfo.subresourceRange.levelCount = (useStaging) ? mipLevels : 1;
		viewCreateInfo.image = image;
		vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view);
		VK_NAME(device->logicalDevice, "loadFromFile::view", view);

		// Update descriptor image info member that can be used for setting up descriptor sets
		updateDescriptor();
	}

	/**
	* Creates a 2D texture from a buffer
	*
	* @param buffer Buffer containing texture data to upload
	* @param bufferSize Size of the buffer in machine units
	* @param width Width of the texture to create
	* @param height Height of the texture to create
	* @param format Vulkan format of the image data stored in the file
	* @param device Vulkan device to create the texture on
	* @param copyQueue Queue used for the texture staging copy commands (must support transfer)
	* @param (Optional) filter Texture filtering for the sampler (defaults to VK_FILTER_LINEAR)
	* @param (Optional) imageUsageFlags Usage flags for the texture's image (defaults to VK_IMAGE_USAGE_SAMPLED_BIT)
	* @param (Optional) imageLayout Usage layout for the texture (defaults VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	*/
	void Texture2D::fromBuffer(void* buffer, VkDeviceSize bufferSize, VkFormat _format,
		uint32_t texWidth, uint32_t texHeight, std::vector<VkBufferImageCopy> mipInfo,VulkanDevice* device, VkQueue copyQueue, VkFilter _filter, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout)
	{
		assert(buffer);

		this->device = device;
		width = texWidth;
		height = texHeight;
		format = _format;
		usage = imageUsageFlags;
		mipLevels =static_cast<uint32_t>(mipInfo.size());
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		//mipLevels = std::floor(std::log2(std::max(texWidth, texHeight))) + 1; //generated mips


		VkCommandBuffer copyCmd = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, device->commandPoolManagers[0].m_commandpool, true);

		
		VkBufferCreateInfo bufferCreateInfo = oGFX::vkutils::inits::bufferCreateInfo(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,bufferSize);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VkBuffer stagingBuffer;
		vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);

		VkMemoryAllocateInfo memAllocInfo = oGFX::vkutils::inits::memoryAllocateInfo();
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = oGFX::FindMemoryTypeIndex(device->physicalDevice,memReqs.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		
		VkDeviceMemory stagingMemory;
		vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory);
		vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0);

		
		uint8_t *data;
		vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data);
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);


		std::vector<VkBufferImageCopy>bufferCopyRegion = mipInfo;

		AllocateImageMemory(device,imageUsageFlags);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 1;

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		oGFX::vkutils::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			bufferCopyRegion.size(), // copy over as many mips as have
			bufferCopyRegion.data()
		);

		// Change texture image layout to shader read after all mip levels have been copied
		this->imageLayout = imageLayout;
		this->currentLayout = imageLayout;
		oGFX::vkutils::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			imageLayout,
			subresourceRange);

		//uboDynamicAlignment = oGFX::vkutils::tools::UniformBufferPaddedSize(sizeof(CB::FrameContextUBO), m_device.properties.limits.minUniformBufferOffsetAlignment);
		//
		//VkDeviceSize vpBufferSize = numCameras * uboDynamicAlignment;
		//
		//oGFX::CreateBuffer(device->m_allocator, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		//	VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
		//	, vpUniformBuffer[i]);


		device->FlushCommandBuffer(copyCmd, copyQueue, device->commandPoolManagers[0].m_commandpool);

		// Clean up staging resources
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);

		

		//// Create image view
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.pNext = NULL;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.levelCount = mipLevels; // generate mip maps will set this
		viewCreateInfo.image = image;
		//vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view);
		//VK_NAME(device->logicalDevice, "fromBuffer::view", view);

		CreateImageView();


		for (size_t i = 0; i < mipLevels; i++)
		{
			viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.subresourceRange.baseMipLevel = i;
			viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;

			//vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &mipChainViews[i]);
			//VK_NAME(device->logicalDevice, "fromBuffer::viewMip", &mipChainViews[i]);
		}

		// Update descriptor image info member that can be used for setting up descriptor sets
		updateDescriptor();
	}

	void Texture2D::AllocateImageMemory(VulkanDevice* device, const VkImageUsageFlags& imageUsageFlags, uint32_t mips)
	{
		mipLevels = mips;

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo = oGFX::vkutils::inits::imageCreateInfo();
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = imageUsageFlags;
		// Ensure that the TRANSFER_DST bit is set for staging
		// if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT))
		{
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		}
		vkCreateImage(device->logicalDevice, &imageCreateInfo, nullptr, &image);
		VK_NAME(device->logicalDevice, name.empty() ? "AllocateImage" : name.c_str(), image);
		VkMemoryRequirements memReqs{};
		vkGetImageMemoryRequirements(device->logicalDevice, image, &memReqs);

		VkMemoryAllocateInfo memAllocInfo = oGFX::vkutils::inits::memoryAllocateInfo();
		memAllocInfo.memoryTypeIndex = oGFX::FindMemoryTypeIndex(device->physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		memAllocInfo.allocationSize = memReqs.size;

		memAllocInfo.memoryTypeIndex = oGFX::FindMemoryTypeIndex(device->physicalDevice, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &deviceMemory);
		vkBindImageMemory(device->logicalDevice, image, deviceMemory, 0);
	}

	void Texture2D::forFrameBuffer(VulkanDevice* device,
		VkFormat _format,
		VkImageUsageFlags imageUsageFlags,
		uint32_t texWidth, uint32_t texHeight,
		bool forFullscr,
		float _renderscale,
		uint32_t _mipLevels,
		VkMemoryPropertyFlags properties,
		VkFilter _filter
	)
	{
		this->device = device;
		targetSwapchain = forFullscr;
		renderScale = _renderscale;
		width = static_cast<uint32_t>(texWidth * renderScale);
		height = static_cast<uint32_t>(texHeight* renderScale);
		format = _format;
		MemProps = properties;
		filter = _filter;

		aspectMask = 0;

		if (imageUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		}
		if (imageUsageFlags & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;// | VK_IMAGE_ASPECT_STENCIL_BIT;
			imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		}

		usage = imageUsageFlags;

		// for blitting
		usage = imageUsageFlags | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		
		assert(aspectMask > 0);

		//uint mipLevels = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;
		mipLevels = 1;

		bool n = name.empty();
		AllocateImageMemory(device, usage);

		CreateImageView();

		filter = _filter;

			
	}

	void Texture2D::Resize(uint32_t texWidth, uint32_t texHeight)
	{
		if (device == nullptr)
			return;

		width = static_cast<uint32_t>(texWidth * renderScale);
		height = static_cast<uint32_t>(texHeight * renderScale);

		VkImageView oldview = view;
		VkDeviceMemory oldMemory = deviceMemory;
		VkImage oldImage = image;

		vkDestroyImageView(device->logicalDevice, oldview, nullptr);
		vkFreeMemory(device->logicalDevice, oldMemory,nullptr);
		vkDestroyImage(device->logicalDevice, oldImage,nullptr);

		bool n = name.empty();

		AllocateImageMemory(device, usage);

		CreateImageView();

	}

	void Texture2D::Update(void* buffer, VkDeviceSize bufferSize, 
		VkFormat _format, uint32_t texWidth, uint32_t texHeight, 
		std::vector<VkBufferImageCopy> mipInfo, VulkanDevice* device,
		VkQueue copyQueue, VkFilter _filter, VkImageUsageFlags imageUsageFlags)
	{
		assert(buffer);

		assert(this->device);
		assert(width == texWidth);
		assert(height == texHeight);
		assert(format == _format);
		assert(usage += imageUsageFlags);
		assert(mipLevels == static_cast<uint32_t>(mipInfo.size()));
		assert(image);

		VkMemoryAllocateInfo memAllocInfo = oGFX::vkutils::inits::memoryAllocateInfo();
		VkMemoryRequirements memReqs;

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY,device->commandPoolManagers[0].m_commandpool, true);

		// Create a host-visible staging buffer that contains the raw image data
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		// This buffer is used as a transfer source for the buffer copy
		VkBufferCreateInfo bufferCreateInfo = oGFX::vkutils::inits::bufferCreateInfo(VK_BUFFER_USAGE_TRANSFER_SRC_BIT,bufferSize);
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		vkCreateBuffer(device->logicalDevice, &bufferCreateInfo, nullptr, &stagingBuffer);

		// Get memory requirements for the staging buffer (alignment, memory type bits)
		vkGetBufferMemoryRequirements(device->logicalDevice, stagingBuffer, &memReqs);

		memAllocInfo.allocationSize = memReqs.size;
		// Get memory type index for a host visible buffer
		memAllocInfo.memoryTypeIndex = oGFX::FindMemoryTypeIndex(device->physicalDevice,memReqs.memoryTypeBits,VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		vkAllocateMemory(device->logicalDevice, &memAllocInfo, nullptr, &stagingMemory);
		vkBindBufferMemory(device->logicalDevice, stagingBuffer, stagingMemory, 0);

		// Copy texture data into staging buffer
		uint8_t *data;
		vkMapMemory(device->logicalDevice, stagingMemory, 0, memReqs.size, 0, (void **)&data);
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(device->logicalDevice, stagingMemory);


		std::vector<VkBufferImageCopy>bufferCopyRegion = mipInfo;

		// be careful
		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = aspectMask;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 1;

		// Image barrier for optimal image (target)
		// Optimal image will be used as destination for the copy
		oGFX::vkutils::tools::setImageLayout(
			copyCmd,
			image,
			this->currentLayout,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		// Copy mip levels from staging buffer
		vkCmdCopyBufferToImage(
			copyCmd,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			mipLevels,
			bufferCopyRegion.data()
		);

		// Change texture image layout to shader read after all mip levels have been copied
		oGFX::vkutils::tools::setImageLayout(
			copyCmd,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			this->currentLayout,
			subresourceRange);

		device->FlushCommandBuffer(copyCmd, copyQueue,device->commandPoolManagers[0].m_commandpool);

		// Clean up staging resources
		vkFreeMemory(device->logicalDevice, stagingMemory, nullptr);
		vkDestroyBuffer(device->logicalDevice, stagingBuffer, nullptr);
	}

	void Texture2D::CreateImageView()
	{
		assert(view == VK_NULL_HANDLE);

		// Create image view
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.pNext = NULL;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange = { aspectMask, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.levelCount = mipLevels;
		viewCreateInfo.image = image;
		VK_CHK(vkCreateImageView(device->logicalDevice, &viewCreateInfo, nullptr, &view));
		VK_NAME(device->logicalDevice, name.empty() ? "CreateImage::view" : name.c_str(), view);
	}

	void TransitionImage(VkCommandBuffer cmd, Texture2D& texture, VkImageLayout targetLayout, uint32_t mipBegin, uint32_t mipEnd)
	{
		//printf("\t Transition::%s -> %s\n", texture.name, oGFX::vkutils::tools::VkImageLayoutString(targetLayout).c_str());

		auto subresrange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		if (texture.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			subresrange =  VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
		}
		
		// default behavior transitiona all mips
		subresrange.baseMipLevel = mipBegin;
		subresrange.levelCount = mipEnd - mipBegin;
		
		if (mipEnd == 0) 
		{// transition some mips
			subresrange.levelCount = texture.mipLevels - mipBegin;
		}

		oGFX::vkutils::tools::insertImageMemoryBarrier(
			cmd,
			texture.image,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			texture.currentLayout,
			targetLayout,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_TRANSFER_BIT,
			subresrange);
		texture.currentLayout = targetLayout;
	}

	void ComputeImageBarrier(VkCommandBuffer cmd, Texture2D& texture, VkImageLayout targetLayout, uint32_t mipBegin, uint32_t mipEnd)
	{
		auto subresrange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		if (texture.format == VK_FORMAT_D32_SFLOAT_S8_UINT)
		{
			subresrange = VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, 0, 1, 0, 1 };
		}
		
		// default behavior transitiona all mips
		subresrange.baseMipLevel = mipBegin;
		subresrange.levelCount = mipEnd - mipBegin;

		if (mipEnd == 0)
		{// transition some mips
			subresrange.levelCount = texture.mipLevels - mipBegin;
		}

		oGFX::vkutils::tools::insertImageMemoryBarrier(
			cmd,
			texture.image,
			VK_ACCESS_MEMORY_WRITE_BIT,
			VK_ACCESS_MEMORY_READ_BIT,
			texture.currentLayout,
			targetLayout,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			subresrange);
		texture.currentLayout = targetLayout;
	}

}
