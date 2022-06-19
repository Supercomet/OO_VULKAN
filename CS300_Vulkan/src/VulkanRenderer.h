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
#include "VulkanTexture.h"
#include "VulkanBuffer.h"
#include "VulkanFramebufferAttachment.h"
#include "GpuVector.h"
#include "gpuCommon.h"
#include "DescriptorBuilder.h"
#include "DescriptorAllocator.h"
#include "DescriptorLayoutCache.h"

#include "Camera.h"

#include "imgui.h"

struct Window;

int Win32SurfaceCreator(ImGuiViewport* vp, ImU64 device, const void* allocator, ImU64* outSurface);


class VulkanRenderer
{
public:

static constexpr int MAX_FRAME_DRAWS = 2;
static constexpr int MAX_OBJECTS = 1024;

#define OBJECT_INSTANCE_COUNT 128


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
	//void CreateDepthBufferImage();
	void CreateFramebuffers(); 
	void CreateCommandBuffers();
	void CreateTextureSampler();

	ImTextureID myImg;
	VulkanFramebufferAttachment offscreenFB;
	VulkanFramebufferAttachment offscreenDepth;
	VkFramebuffer offscreenFramebuffer;
	VkRenderPass offscreenPass;
	void CreateOffscreenPass();
	void CreateOffscreenFB();
	void ResizeOffscreenFB();


	inline static VulkanInstance m_instance{};
	inline static VulkanDevice m_device{};
	inline static VulkanSwapchain m_swapchain{};
	inline static std::vector<VkFramebuffer> swapChainFramebuffers;
	inline static uint32_t swapchainIdx{0};

	//ImTextureID deferredImg[4]{};
	//VulkanFramebufferAttachment att_albedo;
	//VulkanFramebufferAttachment att_position;
	//VulkanFramebufferAttachment att_normal;
	//VulkanFramebufferAttachment att_depth;
	//VkFramebuffer deferredFB;
	//VkSampler deferredSampler;
	//VkRenderPass deferredPass;
	//VkPipelineLayout deferredPipeLayout; // stealing from pipeline
	//VkPipeline deferredPipe;
	inline static VkDescriptorSet deferredSet;
	inline static VkDescriptorSetLayout deferredSetLayout;

	//void CreateDeferredPass();
	//void CreateDeferredPipeline();
	//void CreateDeferredFB();
	void ResizeDeferredFB();
	void DeferredPass();
	//void CleanupDeferredStuff();

	struct Light {
		glm::vec4 position;
		glm::vec3 color;
		float radius;
	};

	struct LightUBO{
		Light lights[6];
		glm::vec4 viewPos;
	} lightUBO;
	float timer{};

	bool deferredRendering = true;
	//VkRenderPass compositionPass;
	//VkPipelineLayout compositionPipeLayout;
	//VkPipeline compositionPipe;

	inline static vk::Buffer lightsBuffer;
	void CreateCompositionBuffers(); 
	void CreateDeferredDescriptorSet();
	void DeferredComposition();
	void UpdateLightBuffer();

	void CreateSynchronisation();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	struct ImGUIStructures
	{
		VkDescriptorPool descriptorPools;
		VkRenderPass renderPass;
		std::vector<VkFramebuffer> buffers;
	};
	ImGUIStructures m_imguiConfig;
	void InitImGUI();
	void ResizeGUIBuffers();
	void DrawGUI();
	void DestroyImGUI();

	void UpdateIndirectCommands();
	void UpdateInstanceData();
	uint32_t objectCount{};
	// Contains the instanced data
	inline static vk::Buffer instanceBuffer;

	bool PrepareFrame();
	void Draw();
	void Present();

	void PrePass();
	void SimplePass();

	void RecordCommands(uint32_t currentImage);
	void UpdateUniformBuffers(uint32_t imageIndex);

	// Immediate command sending helper
	VkCommandBuffer beginSingleTimeCommands();
	// Immediate command sending helper
	void endSingleTimeCommands(VkCommandBuffer commandBuffer);

	uint32_t CreateTexture(uint32_t width, uint32_t height,unsigned char* imgData);
	uint32_t CreateTexture(const std::string& fileName);

	GpuVector<oGFX::Vertex> g_debugDrawVertBuffer{ &m_device };
	GpuVector<uint32_t> g_debugDrawIndxBuffer{ &m_device };
	std::vector<oGFX::Vertex> g_debugDrawVerts;
	std::vector<uint32_t> g_debugDrawIndices;
	void InitDebugBuffers();
	void UpdateDebugBuffers();
	void DestroyDebugBuffers();

	VkRenderPass debugRenderpass;
	void CreateDebugRenderpass();
	void DebugPass();

	inline static GpuVector<oGFX::Vertex> g_vertexBuffer{&VulkanRenderer::m_device};
	inline static GpuVector<uint32_t> g_indexBuffer{ &VulkanRenderer::m_device };
	size_t g_indexOffset{};
	size_t g_vertexOffset{};
	Model* LoadMeshFromFile(const std::string& file);
	uint32_t LoadMeshFromBuffers(std::vector<oGFX::Vertex>& vertex,std::vector<uint32_t>& indices, gfxModel* model);
	void SetMeshTextures(uint32_t modelID,uint32_t alb, uint32_t norm, uint32_t occlu, uint32_t rough);

	void UpdateModel(int modelId, glm::mat4 newModel);

	bool ResizeSwapchain();

	inline static Window* windowPtr{nullptr};


	//textures
	std::vector<VkImage> textureImages;
	std::vector<VkDeviceMemory> textureImageMemory;
	std::vector<VkImageView> textureImageViews;

	std::vector<vk::Texture2D> newTextures;

	// - Synchronisation
	std::vector<VkSemaphore> imageAvailable;
	std::vector<VkSemaphore> renderFinished;
	std::vector<VkFence> drawFences;


	// - Pipeline
	VkPipeline graphicsPipeline{};
	VkPipeline wirePipeline{};
	VkPipeline linesPipeline{};
	//VkPipelineLayout pipelineLayout{};
	inline static VkRenderPass defaultRenderPass{};

	inline static vk::Buffer indirectCommandsBuffer{};
	inline static VkPipeline indirectPipeline{};
	inline static VkPipelineLayout indirectPipeLayout{};
	inline static uint32_t indirectDrawCount{};
	//uint32_t indirectDebugDrawCount{};


	// - Descriptors
	VkDescriptorSetLayout descriptorSetLayout{};
	VkDescriptorSetLayout samplerSetLayout{};
	struct PushConstData
	{
		glm::mat4 xform{};
		glm::vec3 light{};
	};
	inline static VkPushConstantRange pushConstantRange{};

	VkDescriptorPool descriptorPool{};
	VkDescriptorPool samplerDescriptorPool{};
	inline static std::vector<VkDescriptorSet> uniformDescriptorSets;
	std::vector<VkDescriptorSet> samplerDescriptorSets;

	std::vector<GPUTransform> gpuTransform;
	GpuVector<GPUTransform> gpuTransformBuffer{&m_device};

	inline static VkDescriptorSet g0_descriptors;
	inline static VkDescriptorSetLayout g0_descriptorsLayout;

	inline static VkDescriptorSet globalSamplers; // big discriptor set of textures

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	inline static DescriptorAllocator DescAlloc;
	inline static DescriptorLayoutCache DescLayoutCache;

	//VkImage depthBufferImage{};
	//VkDeviceMemory depthBufferImageMemory{};
	//VkImageView depthBufferImageView{};

	VkSampler textureSampler{};

	inline static std::vector<VkCommandBuffer> commandBuffers;

	// Store the indirect draw commands containing index offsets and instance count per object
	inline static std::vector<VkDrawIndexedIndirectCommand> indirectCommands;
	std::vector<VkDrawIndexedIndirectCommand> indirectDebugCommands;

	//Scene objects
	inline static std::vector<gfxModel> models;

	uint32_t currentFrame = 0;

	struct LightData
	{
		glm::vec3 position{};
	};

	uint64_t uboDynamicAlignment;
	uint32_t numCameras;
	struct UboViewProjection
	{
		glm::mat4 projection{ 1.0f };
		glm::mat4 view{ 1.0f };
		glm::vec4 cameraPos{ 1.0f };
	} uboViewProjection;

	bool resizeSwapchain = false;

	LightData light;
	Camera camera;

	public:
	struct EntityDetails
	{
		uint32_t modelID{};
		glm::vec3 pos{};
		glm::vec3 scale{1.0f};
		float rot{};
		glm::vec3 rotVec{0.0f,1.0f,0.0f};
	};
	inline static std::vector<EntityDetails> entities;

	static VkPipelineShaderStageCreateInfo LoadShader(VulkanDevice& device, const std::string& fileName, VkShaderStageFlagBits stage);
	private:
		uint32_t CreateTextureImage(const std::string& fileName);
		uint32_t CreateTextureImage(const oGFX::FileImageData& imageInfo);
		uint32_t CreateTextureDescriptor(VkImageView textureImage);
		uint32_t CreateTextureDescriptor(vk::Texture2D texture);		

};

