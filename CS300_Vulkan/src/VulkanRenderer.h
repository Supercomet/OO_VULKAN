#pragma once

#include <vector>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "MeshModel.h"

#include "VulkanUtils.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "gpuCommon.h"

struct Window;

class VulkanRenderer
{
public:

static constexpr int MAX_FRAME_DRAWS = 2;
static constexpr int MAX_OBJECTS = 1024;

	~VulkanRenderer();

	void CreateInstance(const oGFX::SetupInfo& setupSpecs);
	void CreateSurface(Window& window);
	void AcquirePhysicalDevice();
	void CreateLogicalDevice();
	void SetupSwapchain();
	void CreateRenderpass();
	void CreateDescriptorSetLayout();

	void CreatePushConstantRange();
	void CreateGraphicsPipeline();
	void CreateDepthBufferImage();
	void CreateFramebuffers(); 
	void CreateCommandPool();
	void CreateCommandBuffers();
	void CreateTextureSampler();

	void CreateSynchronisation();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void Draw();
	void RecordCommands(uint32_t currentImage);
	void UpdateUniformBuffers(uint32_t imageIndex);

	uint32_t CreateMeshModel(std::vector<oGFX::Vertex>& vertices,std::vector<uint32_t>& indices);
	uint32_t CreateTexture(uint32_t width, uint32_t height,const char* imgData);

	bool ResizeSwapchain();

	Window* windowPtr{nullptr};

	//Scene objects
	std::vector<MeshModel> modelList;

	//textures
	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	// - Synchronisation
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;

	VulkanInstance m_instance{};
	VulkanDevice m_device{};
	VulkanSwapchain m_swapchain{};
	std::vector<VkFramebuffer> swapChainFramebuffers;

	// - Pipeline
	VkPipeline graphicsPipeline{};
	VkPipelineLayout pipelineLayout{};
	VkRenderPass renderPass{};

	// - Pools
	VkCommandPool graphicsCommandPool{};

	// - Descriptors
	VkDescriptorSetLayout descriptorSetLayout{};
	VkDescriptorSetLayout samplerSetLayout{};
	VkPushConstantRange pushConstantRange{};

	VkDescriptorPool descriptorPool{};
	VkDescriptorPool samplerDescriptorPool{};
	std::vector<VkDescriptorSet> descriptorSets;
	std::vector<VkDescriptorSet> samplerDescriptorSets;

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;


	VkImage depthBufferImage{};
	VkDeviceMemory depthBufferImageMemory{};
	VkImageView depthBufferImageView{};

	VkSampler textureSampler{};

	std::vector<VkCommandBuffer> commandBuffers;

	uint32_t currentFrame = 0;

	struct Model
	{
		float mdl[16];
	};

	struct UboViewProjection
	{
		oGFX::mat4 projection;
		oGFX::mat4 view;
	} uboViewProjection;

	bool resizeSwapchain = false;

	private:
		uint32_t CreateTextureImage(uint32_t width, uint32_t height, const char* imgData);
		uint32_t CreateTextureDescriptor(VkImageView textureImage);
};

