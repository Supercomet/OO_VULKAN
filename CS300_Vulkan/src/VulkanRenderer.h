#pragma once

#include <vector>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "MeshModel.h"

#include "VulkanUtils.h"
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "gpuCommon.h"

#include "Camera.h"

struct Window;

class VulkanRenderer
{
public:

static constexpr int MAX_FRAME_DRAWS = 2;
static constexpr int MAX_OBJECTS = 1024;

	~VulkanRenderer();

	void Init(const oGFX::SetupInfo& setupSpecs, Window& window);

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
	void CreateCommandBuffers();
	void CreateTextureSampler();

	void CreateSynchronisation();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	void UpdateIndirectCommands();

	void Draw();
	void RecordCommands(uint32_t currentImage);
	void UpdateUniformBuffers(uint32_t imageIndex);

	uint32_t CreateMeshModel(std::vector<oGFX::Vertex>& vertices,std::vector<uint32_t>& indices);
	uint32_t CreateMeshModel(const std::string& file);
	uint32_t CreateTexture(uint32_t width, uint32_t height,const unsigned char* imgData);
	uint32_t CreateTexture(const std::string& fileName);

	uint32_t LoadMeshFromFile(const std::string& file);

	void UpdateModel(int modelId, glm::mat4 newModel);

	bool ResizeSwapchain();

	Window* windowPtr{nullptr};

	//Scene objects
	std::vector<MeshContainer> modelList;

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

	vk::Buffer indirectCommandsBuffer{};
	VkPipeline indirectPipeline{};
	VkPipelineLayout indirectPipeLayout{};
	uint32_t indirectDrawCount{};


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

	// Store the indirect draw commands containing index offsets and instance count per object
	std::vector<VkDrawIndexedIndirectCommand> indirectCommands;

	std::vector<Model> models;

	uint32_t currentFrame = 0;

	struct Transform
	{
		glm::mat4 mdl{ 1.0f };
	};

	struct UboViewProjection
	{
		glm::mat4 projection{ 1.0f };
		glm::mat4 view{ 1.0f };
	} uboViewProjection;

	bool resizeSwapchain = false;

	Camera camera;

	private:
		uint32_t CreateTextureImage(const std::string& fileName);
		uint32_t CreateTextureImage(uint32_t width, uint32_t height, const unsigned char* imgData);
		uint32_t CreateTextureDescriptor(VkImageView textureImage);
};

