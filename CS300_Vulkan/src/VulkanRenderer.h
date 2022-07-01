#pragma once

#include <vector>
#include <string>

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include "MeshModel.h"

#include "GfxTypes.h"
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
#include "Geometry.h"

#include "Camera.h"

#include "imgui.h"

#include "GfxSampler.h"

struct Window;

int Win32SurfaceCreator(ImGuiViewport* vp, ImU64 device, const void* allocator, ImU64* outSurface);


class VulkanRenderer
{
public:

static constexpr int MAX_FRAME_DRAWS = 2;
static constexpr int MAX_OBJECTS = 1024;

#define OBJECT_INSTANCE_COUNT 128


inline static PFN_vkDebugMarkerSetObjectNameEXT pfnDebugMarkerSetObjectName{ nullptr };

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

	inline static VkDescriptorSet deferredSet;
	inline static VkDescriptorSetLayout deferredSetLayout;

	void ResizeDeferredFB();
	void DeferredPass();

	struct Light {
		glm::vec4 position{};
		glm::vec3 color{1.0f};
		float radius{1.0f};
	};

	struct LightUBO{
		Light lights[6];
		glm::vec4 viewPos;
	} lightUBO{};
	float timer{};

	bool deferredRendering = true;

	inline static vk::Buffer lightsBuffer;
	void CreateCompositionBuffers(); 
	void DeferredComposition();
	void UpdateLightBuffer();

	void CreateSynchronisation();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSets();

	struct ImGUIStructures
	{
		VkDescriptorPool descriptorPools{};
		VkRenderPass renderPass{};
		std::vector<VkFramebuffer> buffers;
	}m_imguiConfig{};
	 
	void InitImGUI();
	void ResizeGUIBuffers();
	void DrawGUI();
	void DestroyImGUI();


	void AddDebugBox(const AABB& aabb, const oGFX::Color& col, size_t loc = -1);
	void AddDebugSphere(const Sphere& aabb, const oGFX::Color& col,size_t loc = -1);

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

	
	void InitDebugBuffers();
	void UpdateDebugBuffers();

	void UpdateTreeBuffers();

	
	void DebugPass();

	struct VertexBufferObject
	{
		GpuVector<oGFX::Vertex> VtxBuffer;
		GpuVector<uint32_t> IdxBuffer;
		size_t VtxOffset{};
		size_t IdxOffset{};
	};

	inline static VertexBufferObject g_MeshBuffers;

	inline static VertexBufferObject g_AABBMeshBuffers;
	std::vector<oGFX::Vertex> g_AABBMeshes;
	inline static VertexBufferObject g_SphereMeshBuffers;
	std::vector<oGFX::Vertex> g_SphereMeshes;


	inline static GpuVector<oGFX::Vertex> g_debugDrawVertBuffer{ &VulkanRenderer::m_device };
	inline static GpuVector<uint32_t> g_debugDrawIndxBuffer{ &VulkanRenderer::m_device };
	std::vector<oGFX::Vertex> g_debugDrawVerts;
	std::vector<uint32_t> g_debugDrawIndices;

	//TEMP
	struct DebugDraw
	{
		GpuVector<oGFX::Vertex> vbo{ &VulkanRenderer::m_device };
		GpuVector<uint32_t> ibo{ &VulkanRenderer::m_device };
		std::vector<oGFX::Vertex> vertex;
		std::vector<uint32_t> indices;
		bool dirty = true;
	};

	inline static bool debug_btmUp_aabb = false;
	inline static bool debug_topDown_aabb = false;
	inline static bool debug_btmUp_sphere = false;
	inline static bool debug_topDown_sphere = false;
	static constexpr size_t g_btmUp_AABB =0;
	static constexpr size_t g_topDwn_AABB =1;
	static constexpr size_t g_btmUp_Sphere=2;
	static constexpr size_t g_topDwn_Sphere=3;

	inline static DebugDraw g_DebugDraws[4];
	void InitTreeDebugDraws();
	void ShutdownTreeDebug();



	Model* LoadMeshFromFile(const std::string& file);
	Model* LoadMeshFromBuffers(std::vector<oGFX::Vertex>& vertex,std::vector<uint32_t>& indices, gfxModel* model);
	void SetMeshTextures(uint32_t modelID,uint32_t alb, uint32_t norm, uint32_t occlu, uint32_t rough);

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
	inline static VkRenderPass defaultRenderPass{};

	inline static vk::Buffer indirectCommandsBuffer{};
	inline static VkPipeline indirectPipeline{};
	inline static VkPipelineLayout indirectPipeLayout{};
	inline static uint32_t indirectDrawCount{};


	// - Descriptors
	inline static VkDescriptorSetLayout descriptorSetLayout{};
	inline static VkDescriptorSetLayout samplerSetLayout{};

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

	// SSBO
	std::vector<GPUTransform> gpuTransform;
	GpuVector<GPUTransform> gpuTransformBuffer{&m_device};

	std::vector<GPUTransform> debugTransform;
	GpuVector<GPUTransform> debugTransformBuffer{&m_device};
	// SSBO

	inline static VkDescriptorSet g0_descriptors;
	inline static VkDescriptorSetLayout g0_descriptorsLayout;

	inline static VkDescriptorSet globalSamplers; // big discriptor set of textures

	std::vector<VkBuffer> vpUniformBuffer;
	std::vector<VkDeviceMemory> vpUniformBufferMemory;

	inline static DescriptorAllocator DescAlloc;
	inline static DescriptorLayoutCache DescLayoutCache;

	GfxSamplerManager samplerManager;

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
		std::string name;
		uint32_t modelID{};
		glm::vec3 pos{};
		glm::vec3 scale{1.0f};
		float rot{};
		glm::vec3 rotVec{0.0f,1.0f,0.0f};
		Sphere sphere;
		AABB aabb;

		template <typename T>
		float GetBVHeuristic();

		template <>
		float GetBVHeuristic<Sphere>()
		{
			return glm::pi<float>()* sphere.radius* sphere.radius;
		}

		template <>
		float GetBVHeuristic<AABB>()
		{
			const auto width  = aabb.halfExt[0];
			const auto height = aabb.halfExt[1];
			const auto depth  = aabb.halfExt[2];
			return 8.0f * ( (width*height)*(width*depth)*(height*depth) );
		}
	};
	inline static std::vector<EntityDetails> entities;

	static VkPipelineShaderStageCreateInfo LoadShader(VulkanDevice& device, const std::string& fileName, VkShaderStageFlagBits stage);
	private:
		uint32_t CreateTextureImage(const std::string& fileName);
		uint32_t CreateTextureImage(const oGFX::FileImageData& imageInfo);
		uint32_t CreateTextureDescriptor(vk::Texture2D texture);		

};

