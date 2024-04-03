#include "TestApplication.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include "gpuCommon.h"
#include "Vulkanrenderer.h"

#include "window.h"
#include "input.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <imgui/imgui.h>
#include <imgui/misc/cpp/imgui_stdlib.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_win32.h>
#include "ImGuizmo.h"

#include "IcoSphereCreator.h"
#include "Tree.h"

#include "Profiling.h"

#include "BoudingVolume.h"
#include "DefaultMeshCreator.h"

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cctype>
#include <thread>
#include <functional>
#include <random> 
#include <numeric>
#include <algorithm>
#include <bitset>
#include <sstream>
#include <barrier>

#include "ImGuizmo.h"
#include "AppUtils.h"

#include "Camera.h"
#include "CameraController.h"
#include "DebugDraw.h"


//#include "anim/SimpleAnim.h" // WR ONLY

static VulkanRenderer* gs_RenderEngine = nullptr;
static GraphicsWorld gs_GraphicsWorld;
static constexpr int hardCodedLights = 6;
static bool drawDebugLights = false;

using BindlessTextureIndex = uint32_t;
static constexpr BindlessTextureIndex INVALID_BINDLESS_TEXTURE_INDEX = 0xFFFFFFFF;

static BindlessTextureIndex gs_WhiteTexture  = INVALID_BINDLESS_TEXTURE_INDEX;
static BindlessTextureIndex gs_BlackTexture  = INVALID_BINDLESS_TEXTURE_INDEX;
static BindlessTextureIndex gs_NormalTexture = INVALID_BINDLESS_TEXTURE_INDEX;
static BindlessTextureIndex gs_PinkTexture   = INVALID_BINDLESS_TEXTURE_INDEX;
static BindlessTextureIndex gs_RedTexture= INVALID_BINDLESS_TEXTURE_INDEX;

static uint32_t globalDionaID{ 0 };

std::array<BindlessTextureIndex, 10> roughness{};
std::array<BindlessTextureIndex, 10> metalic{};
uint32_t beginSpheres;
uint32_t endSpheres;

struct DummyTestMaterial
{
    BindlessTextureIndex albedo;
    BindlessTextureIndex normal;
    BindlessTextureIndex roughness;
    BindlessTextureIndex metallic;
};

// Temporary solution to assign random numbers... Shamelessly stolen from Intel.
inline uint32_t FastRandomMagic()
{
	static uint32_t seed = 0xDEADBEEF;
	seed = (214013 * seed + 2531011);
	return (seed >> 16) & 0x7FFF;
};

void TestApplication::Init()
{

}

void InitLights(int32_t* someLights);

static uint32_t gs_ModelID_Box = 0;
bool resetBones = false;

bool s_freezeLight = true;
bool s_freezeSphere = true;
bool s_boolCamera = 0;

// this is to pretend we have an ECS system
struct EntityInfo
{
    //helper to set the first bitset for trivial mesh
    EntityInfo() {
    }

    std::string name;
    glm::vec3 position{};
    glm::vec3 scale{1.0f};
    float rot{};
    glm::vec3 rotVec{0.0f,1.0f,0.0f};

    uint32_t modelID{}; // Index for the mesh
    uint32_t entityID{}; // Unique ID for this entity instance // THIS IS THE ECS UUID

    // Very ghetto... To move out to proper material system...
    // Actually 16 bits is enough...
    uint32_t bindlessGlobalTextureIndex_Albedo{ 0xFFFFFFFF };
    uint32_t bindlessGlobalTextureIndex_Normal{ 0xFFFFFFFF };
    uint32_t bindlessGlobalTextureIndex_Roughness{ 0xFFFFFFFF };
    uint32_t bindlessGlobalTextureIndex_Metallic{ 0xFFFFFFFF };
    uint32_t bindlessGlobalTextureIndex_Emissive{ 0xFFFFFFFF };
    uint8_t instanceData{ 0 };

    int32_t gfxID{}; // gfxworld id
    uint32_t submesh{ 0 };

    ObjectInstanceFlags flags{static_cast<ObjectInstanceFlags>(ObjectInstanceFlags::RENDER_ENABLED 
        | ObjectInstanceFlags::SHADOW_RECEIVER 
        | ObjectInstanceFlags::SHADOW_CASTER
        | ObjectInstanceFlags::SHADOW_ENABLED)};

    oGFX::CPUSkeletonInstance* localSkeleton{nullptr};

    mat4 getLocalToWorld()
    {
        glm::mat4 xform{ 1.0f };
        xform = glm::translate(xform, position);
        xform = glm::orientation( glm::normalize(rotVec), vec3(0,1,0)) * xform;
        xform = glm::scale(xform, scale);
        //xform = glm::rotate(xform, glm::radians(rot), rotVec);
        return xform;
    }

    void SyncToGraphicsWorld()
    {
        auto& gfxWorldObjectInstance = gs_GraphicsWorld.GetObjectInstance(gfxID);
        gfxWorldObjectInstance.localToWorld = getLocalToWorld();
        gfxWorldObjectInstance.flags = flags;
        gfxWorldObjectInstance.SetDirty();
    }
};

class GizmoContext
{
public:
    void SelectEntityInfo(EntityInfo* ptr)
    {
        FreeSelection();
        m_EntityInfo = ptr;
    }

    void SelectVector3Property(float* ptr)
    {
        m_Vector3Property = ptr;
    }

    EntityInfo* GetSelectedEntityInfo() const { return m_EntityInfo; }
    float* GetSelectedVector3Property() const { return m_Vector3Property; }
   
    void FreeSelection()
    {
        m_EntityInfo = nullptr;
        m_Vector3Property = nullptr;
    }

    bool AnyPropertySelected() { return m_Vector3Property; }

    void SetVector3Property(const glm::vec3& value)
    {
        if (m_Vector3Property == nullptr)
        {
            std::cout << "[Gizmo] Trying to set value on null m_Vector3Property" << std::endl;
            return;
        }
        glm::vec3* target = (glm::vec3*)m_Vector3Property;
        *target = value;
    }

private:
    EntityInfo* m_EntityInfo{ nullptr };
    float* m_Vector3Property{ nullptr };
};

void CreateGraphicsEntityHelper(EntityInfo& ei)
{
    oGFX::AABB ab;

    //UpdateBV(gs_RenderEngine->models[e.modelID].cpuModel, e);
    ObjectInstance o{};
	o.name = ei.name;
	o.bindlessGlobalTextureIndex_Albedo = ei.bindlessGlobalTextureIndex_Albedo;
	o.bindlessGlobalTextureIndex_Normal = ei.bindlessGlobalTextureIndex_Normal;
	o.bindlessGlobalTextureIndex_Roughness = ei.bindlessGlobalTextureIndex_Roughness;
	o.bindlessGlobalTextureIndex_Metallic = ei.bindlessGlobalTextureIndex_Metallic;
	o.bindlessGlobalTextureIndex_Emissive= ei.bindlessGlobalTextureIndex_Emissive;
    o.instanceData = ei.instanceData;
	o.localToWorld = ei.getLocalToWorld();
	o.modelID = ei.modelID;
	o.entityID = ei.entityID;
    o.flags = ei.flags;
    o.submesh = ei.submesh;

	auto id = gs_GraphicsWorld.CreateObjectInstance(o);
    // assign id
    ei.gfxID = id;
}

static std::vector<EntityInfo> entities;

static CameraController gs_CameraController;

static GizmoContext gs_GizmoContext{};

std::unique_ptr<ModelFileResource> gs_test_scene;
std::unique_ptr<ModelFileResource> character_diona;

void ResetSpheres() 
{
    const float gridSize = 1.3f;
    const float halfGrid = roughness.size() * gridSize / 2.0f;
    for (size_t i = 0; i < roughness.size(); i++)
    {
        for (size_t y = 0; y < roughness.size(); y++)
        {
            auto idx = beginSpheres +y + i * roughness.size();
            auto& ed = entities[idx];
            ed.position = { gridSize * i - halfGrid,gridSize * y - halfGrid + 3.5f,-5.0f };
            ed.scale = glm::vec3(1.0f);
            ed.SyncToGraphicsWorld();
            
        }
    }
   
   
}
OO_OPTIMIZE_OFF
void TestApplication::Run()
{
    gs_RenderEngine = VulkanRenderer::get();

    //----------------------------------------------------------------------------------------------------
    // Setup App Window
    //----------------------------------------------------------------------------------------------------

    AppWindowSizeTypes appWindowSizeType = AppWindowSizeTypes::HD_900P_16_10;
    m_WindowSize = gs_AppWindowSizes[(int)appWindowSizeType];
    Window mainWindow(m_WindowSize.x, m_WindowSize.y);
    mainWindow.Init();

    oGFX::SetupInfo setupSpec;

    //setupSpec.debug = BoolQueryUser("Do you want debugging?");
    //setupSpec.renderDoc = BoolQueryUser("Do you want renderdoc?");
    setupSpec.debug = true;
    setupSpec.renderDoc = true;

    //----------------------------------------------------------------------------------------------------
    // Setup Graphics Engine
    //----------------------------------------------------------------------------------------------------

    
   
    auto result = gs_RenderEngine->Init(setupSpec, mainWindow);

    if (result != oGFX::SUCCESS_VAL)
    {
        std::cout << "Failed to create Vulkan instance!" << std::endl;
        auto c = getchar();
        return;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows


    ImGui::StyleColorsDark(); // Setup Dear ImGui style
    gs_RenderEngine->InitImGUI();

    std::cout << "Created Vulkan instance!" << std::endl;
   
  
    std::unique_ptr<oGFX::Font>testFont (gs_RenderEngine->LoadFont("../Application/fonts/Roboto-Medium.ttf"));
   

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Textures
    //----------------------------------------------------------------------------------------------------
    gs_WhiteTexture = gs_RenderEngine->whiteTextureID;
    gs_BlackTexture = gs_RenderEngine->blackTextureID;
    gs_NormalTexture = gs_RenderEngine->normalTextureID;
    gs_PinkTexture = gs_RenderEngine->pinkTextureID;

    uint32_t colRed = 0xFF0000FF;
    gs_RedTexture = gs_RenderEngine->CreateTexture(1, 1, (unsigned char*) & colRed);

    const BindlessTextureIndex diffuseTexture0 = gs_RenderEngine->CreateTexture("Textures/7/d.png");
    const BindlessTextureIndex diffuseTexture1 = gs_RenderEngine->CreateTexture("Textures/8/d.png");
    const BindlessTextureIndex diffuseTexture2 = gs_RenderEngine->CreateTexture("Textures/13/d.png");
    const BindlessTextureIndex diffuseTexture3 = gs_RenderEngine->CreateTexture("Textures/23/d.png");

    const BindlessTextureIndex roughnessTexture0 = gs_RenderEngine->CreateTexture("Textures/7/r.png");
    const BindlessTextureIndex roughnessTexture1 = gs_RenderEngine->CreateTexture("Textures/8/r.png");
    const BindlessTextureIndex roughnessTexture2 = gs_RenderEngine->CreateTexture("Textures/13/r.png");
    const BindlessTextureIndex roughnessTexture3 = gs_RenderEngine->CreateTexture("Textures/23/r.png");

    const BindlessTextureIndex d0 = gs_RenderEngine->CreateTexture("Textures/13/d.png");
    const BindlessTextureIndex m0 = gs_RenderEngine->CreateTexture("Textures/13/m.png");
    const BindlessTextureIndex n0 = gs_RenderEngine->CreateTexture("Textures/13/n.png");
    const BindlessTextureIndex r0 = gs_RenderEngine->CreateTexture("Textures/13/r.png");

    const BindlessTextureIndex normalTexture0 = gs_RenderEngine->CreateTexture("Textures/7/n.png");

    const BindlessTextureIndex cubeTexture = gs_RenderEngine->CreateCubeMapTexture("Textures/viking");
    

    for (size_t i = 0; i < roughness.size(); i++)
    {
        float val = 1.0f / (roughness.size()-1);
        uint32_t byteVal = (uint32_t)(val * i * 0xFF);
        uint32_t colVal = byteVal << 24 | byteVal << 16 | byteVal << 8 | byteVal;

        roughness[i] = gs_RenderEngine->CreateTexture(1, 1, (unsigned char*)&colVal);
        metalic[i] = gs_RenderEngine->CreateTexture(1, 1, (unsigned char*)&colVal);
    }
    //----------------------------------------------------------------------------------------------------
    // Setup Initial Models
    //----------------------------------------------------------------------------------------------------

    auto defaultPlaneMesh = CreateDefaultPlaneXZMesh();
    auto defaultCubeMesh = CreateDefaultCubeMesh();
    auto defaultSphere = icosahedron::make_icosphere(3);

    std::unique_ptr<ModelFileResource> model_plane{ gs_RenderEngine->LoadMeshFromBuffers(defaultPlaneMesh.m_VertexBuffer, defaultPlaneMesh.m_IndexBuffer, nullptr) };
    std::unique_ptr<ModelFileResource> model_box{ gs_RenderEngine->LoadMeshFromBuffers(defaultCubeMesh.m_VertexBuffer, defaultCubeMesh.m_IndexBuffer, nullptr) };
    std::unique_ptr<ModelFileResource> model_sphere{nullptr};
    {
        std::vector<uint32_t> indices;
        indices.reserve(defaultSphere.second.size() * 3);
        for (size_t i = 0; i < defaultSphere.second.size(); i++)
        {
            auto Tv = defaultSphere.second[i].vertex;
            for (size_t x = 0; x < 3; x++)
            {
                indices.push_back(Tv[x]);
            }
        }
        std::vector<oGFX::Vertex> vertices;
        vertices.resize(defaultSphere.first.size());
        for (size_t i = 0; i < defaultSphere.first.size(); i++)
        {
            vertices[i].pos = defaultSphere.first[i];
            vertices[i].norm = vec3{ 0.0f };
        }
        for (size_t i = 0; i < indices.size(); i+=3)
        {
            std::swap(indices[i], indices[i + 1]);
            auto idx0 = indices[i];
            auto idx1 = indices[i+1];
            auto idx2 = indices[i+2];

            vec3 norm = glm::cross(vertices[idx1].pos - vertices[idx0].pos,vertices[idx2].pos - vertices[idx0].pos);
            vertices[idx0].norm += norm;
            vertices[idx1].norm += norm;
            vertices[idx2].norm += norm;
            vertices[idx0].tangent = vertices[idx2].pos - vertices[idx1].pos;
            vertices[idx1].tangent = vertices[idx2].pos - vertices[idx1].pos;
            vertices[idx2].tangent = vertices[idx2].pos - vertices[idx1].pos;
            
            
        }

        for (auto& v : vertices)
        {
            v.norm = glm::normalize(v.norm);
            v.tangent = glm::normalize(v.tangent);

            v.tex.x = 0.5f + glm::atan2(v.pos.z, v.pos.x) / 2.0f * glm::pi<float>();
            v.tex.y = 0.5f + glm::asin(v.pos.y) / glm::pi<float>();
        }        
        model_sphere.reset(gs_RenderEngine->LoadMeshFromBuffers(vertices, indices, nullptr));
    }
    gs_ModelID_Box = model_box->indices.front();

    character_diona.reset(gs_RenderEngine->LoadModelFromFile("../Application/models/MainChar_Idle.fbx"));
    //std::unique_ptr<ModelFileResource> character_qiqi{ gs_RenderEngine->LoadModelFromFile("../Application/models/character/qiqi.fbx") };
    std::unique_ptr<ModelFileResource> character_qiqi{ nullptr};

    /* // WIP
    std::unique_ptr<ModelFileResource> alibaba{ gs_RenderEngine->LoadModelFromFile("../Application/models/anim/AnimationTest_Box.fbx") };
    std::unique_ptr<simpleanim::SkinnedMesh> skinnedMesh_alibaba = std::make_unique<simpleanim::SkinnedMesh>();
    simpleanim::LoadModelFromFile_Skeleton("../Application/models/anim/AnimationTest_Box.fbx", simpleanim::LoadingConfig{}, alibaba.get(), skinnedMesh_alibaba.get());
    */
    /* // WIP
    std::unique_ptr<ModelFileResource> character_dori{ gs_RenderEngine->LoadModelFromFile("../Application/models/character/dori.fbx") };
    std::unique_ptr<SkinnedMesh> skinnedMesh_dori = std::make_unique<SkinnedMesh>();
    LoadModelFromFile_Skeleton("../Application/models/character/dori.fbx", LoadingConfig{}, character_dori.get(), skinnedMesh_dori.get());
    */
    //----------------------------------------------------------------------------------------------------
    // Setup Initial Scene Objects
    //----------------------------------------------------------------------------------------------------

    // Comment/Uncomment as needed
    gs_test_scene.reset( gs_RenderEngine->LoadModelFromFile("../Application/models/scene/Sponza/gltf/Sponza.gltf") );
    //gs_test_scene.reset( gs_RenderEngine->LoadModelFromFile("../Application/models/scene/Bistro/BistroExterior.fbx") );
    if(gs_test_scene) LoadMeshTextures(gs_test_scene.get());
    if(gs_test_scene) ProcessModelScene(gs_test_scene.get());

    std::array<uint32_t, 4> diffuseBindlessTextureIndexes =
    {
        diffuseTexture0, diffuseTexture1, diffuseTexture2, diffuseTexture3
    };

    std::array<uint32_t, 4> roughnessBindlessTextureIndexes =
    {
        roughnessTexture0, roughnessTexture1, roughnessTexture2, roughnessTexture3
    };

    {
        auto& ed = entities.emplace_back(EntityInfo{});
        ed.name = "Ground_Plane";
        ed.entityID = FastRandomMagic();
        ed.modelID = model_plane->meshResource;
        ed.flags = ObjectInstanceFlags(static_cast<uint32_t>(ed.flags));
        ed.position = { 0.0f,-0.5f,0.0f };
        ed.scale = { 15.0f,1.0f,15.0f };

        ed.bindlessGlobalTextureIndex_Albedo    = testFont->m_atlasID;
        ed.bindlessGlobalTextureIndex_Normal    = n0;
        ed.bindlessGlobalTextureIndex_Metallic  = m0;
        ed.bindlessGlobalTextureIndex_Roughness = r0;
        ed.bindlessGlobalTextureIndex_Emissive= r0;
    }

    //{
    //    auto& ed = entities.emplace_back(EntityInfo{});
    //    ed.name = "Sponza";
    //    ed.entityID = FastRandomMagic();
    //    ed.modelID = gs_test_scene->meshResource;
    //    ed.flags = ObjectInstanceFlags(static_cast<uint32_t>(ed.flags));
    //    ed.position = { 0.0f,-0.5f,0.0f };
    //    ed.scale = { 15.0f,1.0f,15.0f };
    //
    //    for (size_t i = 0; i < gs_test_scene->numSubmesh; i++)
    //    {
    //        ed.submeshes[i] = 1;
    //    }       
    //
    //    ed.bindlessGlobalTextureIndex_Albedo = gs_WhiteTexture;
    //    ed.bindlessGlobalTextureIndex_Normal = n0;
    //    ed.bindlessGlobalTextureIndex_Metallic = m0;
    //    ed.bindlessGlobalTextureIndex_Roughness = r0;
    //    ed.bindlessGlobalTextureIndex_Emissive = r0;
    //}

    uint32_t uiID = CreateTextHelper(glm::mat4(1.0f), std::string("123 Test\nNew Line"), testFont.get());

   
    {        
        globalDionaID = (uint32_t)entities.size();
        auto& ed = entities.emplace_back(EntityInfo{});
        ed.modelID = character_diona->meshResource;
        ed.name = "diona";
        ed.entityID = FastRandomMagic();
        ed.position = { 3.0f,0.0f,0.0f };
        ed.scale = { 0.1f,0.1f,0.1f };
        ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        ed.flags = ObjectInstanceFlags((uint32_t)ed.flags|(uint32_t)ObjectInstanceFlags::SKINNED);
        ed.flags = ed.flags | ObjectInstanceFlags::SHADOW_CASTER;
        ed.flags = ed.flags | ObjectInstanceFlags::SHADOW_ENABLED;
        ed.flags = ed.flags | ObjectInstanceFlags::SHADOW_RECEIVER;
        
    }
    std::unique_ptr<oGFX::CPUSkeletonInstance> scopedPtr{ gs_RenderEngine->CreateSkeletonInstance(entities[globalDionaID].modelID) };
    entities[globalDionaID].localSkeleton = scopedPtr.get();
    
    uint32_t LSphere;
    uint32_t bigSphere;
    if (m_TestDebugFrustum) {
       
        {
            LSphere = (uint32_t)entities.size();
            auto& ed = entities.emplace_back(EntityInfo{});
            ed.name = "Box";
            ed.entityID = FastRandomMagic();
            ed.modelID = model_sphere->meshResource;
            ed.position = { 0.0f,0.0f,0.0f };
            ed.scale = glm::vec3{ 0.5f };

            ed.bindlessGlobalTextureIndex_Albedo = diffuseBindlessTextureIndexes[0];
            ed.bindlessGlobalTextureIndex_Roughness = roughnessBindlessTextureIndexes[0];
            ed.bindlessGlobalTextureIndex_Normal = normalTexture0;
        }

      
        {
            bigSphere = (uint32_t)entities.size();
            auto& ed = entities.emplace_back(EntityInfo{});
            ed.name = "Rsphere";
            ed.entityID = FastRandomMagic();
            ed.modelID = model_sphere->meshResource;
            ed.position = { 0.0f,0.0f,0.0f };
            ed.scale = glm::vec3{ 1.0f };

            ed.bindlessGlobalTextureIndex_Albedo = diffuseBindlessTextureIndexes[0];
            ed.bindlessGlobalTextureIndex_Roughness = roughnessBindlessTextureIndexes[0];
            ed.bindlessGlobalTextureIndex_Normal = normalTexture0;
        }
    }
    


    const float gridSize = 1.3f;
    const float halfGrid = roughness.size() * gridSize / 2.0f;
    beginSpheres = (uint32_t)entities.size();
    for (size_t i = 0; i < roughness.size(); i++)
    {
        for (size_t y = 0; y < metalic.size(); y++)
        {
            auto& ed = entities.emplace_back(EntityInfo{});

            std::stringstream ss;
            ss << "Sphere_M[" << y << "]R[" << i << "]";
            ed.name = std::string("Sphere_M") + std::to_string(i * metalic.size() + y);
            ed.name = ss.str();
            ed.entityID = FastRandomMagic();
            ed.modelID = model_sphere->meshResource;
            //ed.flags = ObjectInstanceFlags(static_cast<uint32_t>(ed.flags)& ~static_cast<uint32_t>(ObjectInstanceFlags::SHADOW_CASTER));

            ed.scale = { 1.0f,1.0f,1.0f };

            ed.bindlessGlobalTextureIndex_Albedo = gs_RedTexture;
            ed.bindlessGlobalTextureIndex_Metallic = metalic[y];
            ed.bindlessGlobalTextureIndex_Roughness = roughness[i];

            std::stringstream name;
            name << "Roughness " << std::setprecision(2) << ((float)i / (roughness.size() - 1))
                << "\nMetallic " << std::setprecision(2) << ((float)y / (roughness.size() - 1));
            auto strpos = glm::vec3{ gridSize * i - halfGrid, gridSize * y - halfGrid + 3.5f - 0.6f, -5.0f };
            CreateTextHelper(glm::translate(strpos), name.str(), testFont.get());

        }
    }
    endSpheres = (uint32_t)entities.size();

    ResetSpheres();

    constexpr float M_PI = glm::pi<float>();
    std::vector<glm::vec4> angles;        
    angles.resize(endSpheres - beginSpheres + 1);
    for (size_t i = 0; i < angles.size(); i++)
    {
        angles[i].x = float(FastRandomMagic()) / UINT32_MAX * M_PI;
        angles[i].y = float(FastRandomMagic()) / UINT32_MAX * M_PI;
        angles[i].z = float(FastRandomMagic()) / UINT32_MAX * M_PI;
        angles[i].w = float(i) / angles.size() * 2.0f;
    }

    /* // WIP
    if (alibaba) 
    {
        auto& ed = entities.emplace_back(EntityInfo{});
        ed.modelID = alibaba->meshResource;
        ed.name = "alibaba";
        ed.entityID = FastRandomMagic();
        ed.position = { 0.0f,0.0f,0.0f };
        ed.scale = { 1.0f,1.0f,1.0f };
        ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        //ed.instanceData = 2;
    }
    */
    
    //if (gs_test_scene)
    //{
    //    EntityHelper(gs_test_scene.get(), gs_test_scene->sceneInfo,true);
    //}

    // Stress test more models
    std::vector<std::unique_ptr<ModelFileResource>> moreModels;
    moreModels.reserve(128);
#define LOAD_MODEL(FILE) moreModels.emplace_back(gs_RenderEngine->LoadModelFromFile("../Application/models/" FILE))
    LOAD_MODEL("arrow.fbx");
    LOAD_MODEL("classroom_door.fbx");
    LOAD_MODEL("cleaning_trolley.fbx");
    LOAD_MODEL("crowd_control_barrier.fbx");
    LOAD_MODEL("english_bond_brick.fbx");
    LOAD_MODEL("filing_cabinet.fbx");
    LOAD_MODEL("garden_bench_01.fbx");
    LOAD_MODEL("greek_column_02.fbx");
    LOAD_MODEL("large_pallet.fbx");
    LOAD_MODEL("location_marker.fbx");
    LOAD_MODEL("moulded_breeze_block_03.fbx");
    LOAD_MODEL("office_desk.fbx");
    LOAD_MODEL("pallet.fbx");
    LOAD_MODEL("park_bench.fbx");
    LOAD_MODEL("torii_gate_01.fbx");
    LOAD_MODEL("tuscan_column.fbx");
    LOAD_MODEL("wooden_crate_02.fbx");
    LOAD_MODEL("wooden_produce_crate.fbx");
    LOAD_MODEL("wood_barrel.fbx");
    LOAD_MODEL("industrial_shelving_02.fbx");
    LOAD_MODEL("pallet_of_pipes.fbx");
    LOAD_MODEL("I_beam.fbx");
#undef LOAD_MODEL
    {
        int counter = 0;
        for (auto& model : moreModels)
        {
            //auto& ed = entities.emplace_back(EntityInfo{});
            //ed.modelID = model->meshResource;
            //ed.name = "model_" + std::to_string(counter);
            //ed.entityID = FastRandomMagic();
            //ed.position = { 2.0f + 2.0 * float(counter % 4), 0.0f, -(2.0f + 2.0 * float(counter / 4)) };
            //ed.scale = { 0.1f,0.1f,0.1f};
            //ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
            //++counter;
        }
    }

    //for (auto& mdl: gs_RenderEngine->g_globalModels)
    //{
    //    std::cout << "model:" << mdl.name << ", " 
    //        << mdl.baseVertex <<", " << mdl.baseVertex+mdl.vertexCount 
    //        <<  " diff-"<< mdl.baseVertex + mdl.vertexCount- mdl.baseVertex  << std::endl;
    //    for (auto& sm : mdl.m_subMeshes)
    //    {
    //        std::cout << "\tsm:" << sm.name << ", " << sm.baseVertex <<", " << sm.baseVertex+sm.vertexCount << std::endl;
    //    }
    //}

    // Transfer to Graphics World
    for (auto& e : entities)
    {
        CreateGraphicsEntityHelper(e);
    }

    int32_t someLights[hardCodedLights];
    InitLights(someLights);

        

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Camera
    //----------------------------------------------------------------------------------------------------
    {
        auto& camera = gs_GraphicsWorld.cameras[0];
        gs_CameraController.SetCamera(&camera);

        camera.m_CameraMovementType = Camera::CameraMovementType::firstperson;
        camera.movementSpeed = 5.0f;
        
        camera.SetRotation(glm::vec3(0.0f, 91.0f, 0.0f));
        camera.SetRotationSpeed(0.5f);
        camera.SetPosition(glm::vec3(0.0f, 2.0f, 10.0f));
        camera.SetAspectRatio((float)mainWindow.m_width / (float)mainWindow.m_height);

        gs_GraphicsWorld.cameras[1] = gs_GraphicsWorld.cameras[0];
    }

    auto lastTime = std::chrono::high_resolution_clock::now();
    

    //----------------------------------------------------------------------------------------------------
    // Set graphics world before rendering
    //----------------------------------------------------------------------------------------------------
    gs_RenderEngine->SetWorld(&gs_GraphicsWorld);
    gs_GraphicsWorld.numCameras = 2;
    gs_GraphicsWorld.cameras[0].SetFarClip(1000.0f);
    gs_RenderEngine->InitWorld(&gs_GraphicsWorld);

    gs_GraphicsWorld.m_HardcodedDecalInstance.position = glm::vec3{ 0.0f,0.0f,0.0f };

    const size_t numThreads = 2;
    std::barrier g_barrier(numThreads);

    bool renderMe = true;
    auto renderWorker = [renderer = gs_RenderEngine,
        &keepRendering = renderMe,
        &barrier = g_barrier
    ]() {
        OPTICK_THREAD("RenderThread");
        auto lastTime = std::chrono::high_resolution_clock::now();
        while (keepRendering == true)
        {
            PROFILE_FRAME("RENDER LOOP");
            auto now = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            renderer->renderClock += deltaTime;
            renderer->deltaTime = deltaTime;



            if (renderer->PrepareFrame() == true)
            {
                renderer->RenderFrame();

                renderer->Present();
               
            }
           
            {
                PROFILE_SCOPED("wait cpu_1");
                barrier.arrive_and_wait();
            }
            {
                PROFILE_SCOPED("wait cpu_2");
                barrier.arrive_and_wait();
            }
        }
      
    };

    std::thread renderThread(renderWorker);
    //----------------------------------------------------------------------------------------------------
    // Application Loop
    //----------------------------------------------------------------------------------------------------
    {
        while (mainWindow.windowShouldClose == false)
        {
            PROFILE_FRAME("MainThread");
            int64_t frame = std::max<int64_t>(int64_t(m_ApplicationFrame) - 1, 0);

			auto now = std::chrono::high_resolution_clock::now();
			float deltaTime = std::chrono::duration<float>(now - lastTime).count();
			lastTime = now;

            // Increment the frame number, increase the running timer, set the delta time.
            m_ApplicationFrame++;
            m_ApplicationTimer += deltaTime;
            m_ApplicationDT = deltaTime;
            // Pass frame information to the render engine
            // gs_RenderEngine->renderClock += deltaTime;
            // gs_RenderEngine->deltaTime = deltaTime;

            //reset keys
            Input::Begin();
            while (Window::PollEvents());

            //------------------------------
            // Camera Controller
            //------------------------------
            // Decoupling camera logic.
            // Graphics should only know the final state before calculating the view & projection matrix.
            {
                auto& camera = gs_GraphicsWorld.cameras[s_boolCamera];

                gs_CameraController.SetCamera(&camera);
                // If the aspect ratio changes, then the projection matrix must be updated correctly...
                camera.SetAspectRatio((float)mainWindow.m_width / (float)mainWindow.m_height);
                gs_CameraController.Update(deltaTime);
            }      


            {
                PROFILE_SCOPED("ImGui::NewFrame");
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
            }    
          

            {
                PROFILE_SCOPED("ImGui::Update");
                ImGui::Begin("Problems");

                static float scales[]{
                    1.0f,
                    1.0f / 1.5f,
                    1.0f / 1.724f, // conform to nvidia's scaling
                    1.0f / 2.0f,
                    1.0f / 3.0f,
                };
                
                static int currItem = 0;
                static const char* presets[]{
                    "NATIVE",
                    "QUALITY",
                    "BALANCED",
                    "PERFORMANCE",
                    "ULTRA PERFORMANCE",
                    "CUSTOM",
                };               
                ImGui::Text("Rendering at {%4d,%4d}", gs_RenderEngine->renderWidth, gs_RenderEngine->renderHeight);
               
                static int upscale_item = 0;
                static const char* upscalers[]{
                    "NONE",
                    "DLSS",
                    "FSR",
                };
                if (ImGui::ListBox("UpscaleType", &upscale_item, upscalers, 3))
                {
                    if (upscale_item < 3)
                    {
                        gs_RenderEngine->SetUpscaler((UPSCALING_TYPE)upscale_item);
                        gs_RenderEngine->UpdateRenderResolution();
                    }
                }

                ImGui::Checkbox("Use Jitter", &gs_RenderEngine->m_useJitter);
                if (ImGui::SliderFloat("RCAS Sharpness", &gs_RenderEngine->rcas_sharpness, 0.0f, 1.0f))
                {

                }
                if (ImGui::ListBox("QualityPresets", &currItem, presets, 6)) 
                {
                    if (currItem < 5) 
                    {
                        gs_RenderEngine->changedRenderResolution = scales[currItem];
                        gs_RenderEngine->SetQuality((UPSCALING_QUALITY)currItem);
                        gs_RenderEngine->UpdateRenderResolution();
                    }
                }
                if (ImGui::SliderFloat("RenderResolution", &gs_RenderEngine->changedRenderResolution, 0.1f, 1.0f))
                {
                    gs_RenderEngine->UpdateRenderResolution();
                    currItem = 5; // set to custom
                }
                if (ImGui::Button("Cause problems"))
                {
                    uint32_t col = 0x00FFFF00;
                    gs_RenderEngine->CreateTexture(1, 1, (unsigned char*)&col);

                }
                {
                    ImGui::PushID("uiID");
                    auto& ent = gs_GraphicsWorld.GetUIInstance(uiID);
                    bool lolthisistext = ent.isText();
                    if (ImGui::Checkbox("isText", &lolthisistext))
                    {
                        ent.SetText(lolthisistext);
                    }
                    ImGui::InputText("Text", &ent.textData);
                    float fontsize = ent.format.fontSize;
                    if (ImGui::DragFloat("FontSize", &fontsize))
                    {
                        ent.format.fontSize = fontsize;
                    }
                    ImGui::ColorPicker4("Colour", glm::value_ptr(ent.colour));
                    ImGui::DragFloat2("Bmin", glm::value_ptr(ent.format.box.min));
                    ImGui::DragFloat2("Bmax", glm::value_ptr(ent.format.box.max));
                    ImGui::PopID();
                    oGFX::AABB aabb;
                    aabb.halfExt = glm::vec3{ (ent.format.box.max - ent.format.box.min) / 2.0f,0.0f };
                    //oGFX::DebugDraw::AddAABB(aabb);
                }
                ImGui::End();

                if (m_TestDebugFrustum) 
                {
                    oGFX::AABB bigBox;
                    bigBox.center = entities[bigSphere].position;
                    bigBox.halfExt = entities[bigSphere].scale / 2.0f;

                    auto inversed_perspectiveRH_ZO = [](float fovRad, float aspect, float n, float f)->glm::mat4 {
                        glm::mat4 result(0.0f);
                        assert(abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
                        float const tanHalfFovy = tan(fovRad / 2.0f);

                        float h = 1.0f / std::tan(fovRad * 0.5f);
                        float w = h / aspect;
                        float a = -n / (f - n);
                        float b = (n * f) / (f - n);
                        result[0][0] = w;
                        result[1][1] = h;
                        result[2][2] = a;
                        result[2][3] = -1.0f;
                        result[3][2] = b;

                        return result;
                        };

               
                    gs_GraphicsWorld.cameras[1].SetFarClip(20.0f);
                    Camera& targetCam = gs_GraphicsWorld.cameras[1];
                    mat4 persp = inversed_perspectiveRH_ZO(glm::radians(targetCam.m_fovDegrees), targetCam.m_aspectRatio, targetCam.m_znear, targetCam.m_zfar);
                    mat4 vp = persp * targetCam.matrices.view;
                    mat4 normalVP = targetCam.GetNonInvProjectionMatrix() * targetCam.matrices.view;
                    oGFX::Frustum forig = gs_GraphicsWorld.cameras[1].GetFrustum();
                    oGFX::Frustum f = oGFX::Frustum::CreateFromViewProj(normalVP);
                    f.pt_top= forig.pt_top;
                    f.pt_bottom= forig.pt_bottom;
                    f.pt_right= forig.pt_right;
                    f.pt_left= forig.pt_left;
                    f.pt_planeFar= forig.pt_planeFar;
                    f.pt_planeNear = forig.pt_planeNear;
                    //oGFX::DebugDraw::DrawCameraFrustrumFromViewProj(vp);
                    oGFX::DebugDraw::DrawCameraFrustrumFromViewProj(normalVP);
               
                    oGFX::coll::Collision result = oGFX::coll::AABBInFrustum(f, bigBox);
                    switch (result) 
                    {
                    case oGFX::coll::INTERSECTS: oGFX::DebugDraw::AddAABB(bigBox,oGFX::Colors::YELLOW);break;
                    case oGFX::coll::CONTAINS:   oGFX::DebugDraw::AddAABB(bigBox,oGFX::Colors::GREEN);break;
                    case oGFX::coll::OUTSIDE:    oGFX::DebugDraw::AddAABB(bigBox);
                    default:break;                        
                    }             

				    TestFrustumCull(f, bigBox);
                }


                if (Input::GetKeyTriggered(KEY_P))
                {
                    int32_t w = (int)m_WindowSize.x;
                    int32_t h = (int)m_WindowSize.y;

                    auto mpos = Input::GetMousePos();
                    mpos /= glm::vec2{ w,h };
                    //std::cout << "Mouse pos [" << mpos.x << "," << mpos.y << "]\n";

                    int32_t col = gs_RenderEngine->GetPixelValue(gs_GraphicsWorld.targetIDs[0], mpos);
                    std::cout << "Mouse picking:: colour val : " << std::hex << col <<std::dec << " | " << col << std::endl;
                }

                if (ImGui::Begin("Main"))
                {
                    if (gs_GraphicsWorld.imguiID[0])
                    {
                        ImGui::Image(gs_GraphicsWorld.imguiID[0], {800,600});
                    }
                }
                ImGui::End();
           
                if (ImGui::Begin("Editor"))
                {
                    static bool ManyCamera{ true };
                    gs_GraphicsWorld.numCameras = 2;
                    ManyCamera = gs_GraphicsWorld.shouldRenderCamera[1];
                    if (ImGui::Checkbox("Many camera", &ManyCamera))
                    {
                        gs_GraphicsWorld.shouldRenderCamera[1] = ManyCamera;
                    }
                    ImGui::Checkbox("EditCam", &s_boolCamera);
                    if (gs_GraphicsWorld.imguiID[1])
                    {
                        ImGui::Image(gs_GraphicsWorld.imguiID[1], {800,600});
                    }
                }
                ImGui::End();
            
            }

            // if (gs_RenderEngine->PrepareFrame() == true)
            // {
            //     PROFILE_SCOPED("gs_RenderEngine->PrepareFrame() == true");
            // 
			if (s_freezeLight == false)
			{
				OmniLightInstance* lights[hardCodedLights];
				for (size_t i = 0; i < hardCodedLights; i++)
				{
					lights[i] = &gs_GraphicsWorld.GetLightInstance(someLights[i]);
				}


				static float lightTimer = 0.0f;
				lightTimer += deltaTime * 0.25f;


				lights[0]->position.x = sin(glm::radians(360.0f * lightTimer)) * 5.0f;
				lights[0]->position.z = cos(glm::radians(360.0f * lightTimer)) * 5.0f;

				lights[1]->position.x = -4.0f + sin(glm::radians(360.0f * lightTimer) + 45.0f) * 2.0f;
				lights[1]->position.z = 0.0f + cos(glm::radians(360.0f * lightTimer) + 45.0f) * 2.0f;

				lights[2]->position.x = 4.0f + sin(glm::radians(360.0f * lightTimer)) * 2.0f;
				lights[2]->position.z = 0.0f + cos(glm::radians(360.0f * lightTimer)) * 2.0f;

				lights[4]->position.x = 0.0f + sin(glm::radians(360.0f * lightTimer + 90.0f)) * 5.0f;
				lights[4]->position.z = 0.0f - cos(glm::radians(360.0f * lightTimer + 45.0f)) * 5.0f;

				lights[5]->position.x = 0.0f + sin(glm::radians(-360.0f * lightTimer + 135.0f)) * 10.0f;
				lights[5]->position.z = 0.0f - cos(glm::radians(-360.0f * lightTimer - 45.0f)) * 10.0f;

			}
            
            if (s_freezeSphere == false) 
            {
                static float ballsTimer = 0.0f;
                ballsTimer += deltaTime * 0.25f;
              
                size_t posIter = 0;
                for (size_t i = beginSpheres; i < endSpheres; i++)
                {
                    angles[i].x += angles[i].w * deltaTime;
                    angles[i].y += angles[i].w * deltaTime;
                    angles[i].z += angles[i].w * deltaTime;
                   
                    if (angles[i].x > 2 * M_PI) {
                        angles[i].x -= 2 * M_PI;
                    }
                    if (angles[i].y > 2 * M_PI) {
                        angles[i].y -= 2 * M_PI;
                    }
                    if (angles[i].z > 2 * M_PI) {
                        angles[i].z -= 2 * M_PI;
                    }
                    float r = 20.0f;
                    std::function<float(float)> funs[] = {
                        [](float x) { return cos(x); }
                    ,[](float x) { return sin(x); } };

                    entities[i].position[(posIter+0)%3] = funs[(posIter+posIter)%2](angles[i].x) * r;
                    entities[i].position[(posIter+1)%3] = sin(angles[i].y) * r;
                    entities[i].position[(posIter+2)%3] = funs[(posIter+2+posIter)%2](angles[i].z) * r;
                    entities[i].scale = glm::vec3(0.2f);
                    entities[i].SyncToGraphicsWorld();
                    ++posIter;
                }
            }
            // 
            //     // We need to test debug draw...
                   RunTest_DebugDraw();
            // 
            //     // Render the frame
            //     gs_RenderEngine->RenderFrame();
            // 
                    // Create a dockspace over the mainviewport so that we can dock stuff
                    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
                        ImGuiDockNodeFlags_PassthruCentralNode // make the dockspace transparent
                        | ImGuiDockNodeFlags_NoDockingInCentralNode // dont allow docking in the central area
                    );
            // 
            //     if (m_ShowImGuiDemoWindow)
            //     {
            //         PROFILE_SCOPED("ImGui::ShowDemoWindow");
            //         ImGui::ShowDemoWindow();
            //     }
            // 
                    // ImGuizmo
                    Tool_HandleGizmoManipulation();
                    
                    // Display ImGui Window
                    {
                        PROFILE_SCOPED("ImGuiSceneHelper");
                        Tool_HandleUI();
                    }
            // 
            //     {
            //         PROFILE_SCOPED("ImGui::Render");
            //         ImGui::Render();  // Rendering UI
            //     }
            //     gs_RenderEngine->DrawGUI();
            // 
            //     gs_RenderEngine->Present();
            // }

            {
                PROFILE_SCOPED("ImGui::Render");
                ImGui::Render();  // Rendering UI
                
                ImDrawData* drawData = ImGui::GetDrawData();               
                gs_RenderEngine->SubmitImguiDrawList(drawData);
            }
            //finish for all windows  

            {
                PROFILE_SCOPED("wait gpu_1"); 
                g_barrier.arrive_and_wait();        
            }
            ImGui::EndFrame();
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }
           
            {
                PROFILE_SCOPED("wait gpu_2");
                g_barrier.arrive_and_wait();
            }

            
            //finish for all windows
            // ImGui::EndFrame();
            // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            // {
            //     ImGui::UpdatePlatformWindows();
            //     ImGui::RenderPlatformWindowsDefault();
            // }

        }
    }

    //----------------------------------------------------------------------------------------------------
    // Application Shutdown
    //----------------------------------------------------------------------------------------------------
    gs_RenderEngine->DestroyImGUI(); 
    ImGui::DestroyContext(ImGui::GetCurrentContext());
   
    renderMe = false;
    g_barrier.arrive_and_drop();
    renderThread.join();

    gs_RenderEngine->DestroyWorld(&gs_GraphicsWorld);


    if (gs_RenderEngine)
        delete gs_RenderEngine;
}

int32_t TestApplication::CreateTextHelper(glm::mat4 xform, std::string str, oGFX::Font* testFont)
{
    int32_t uiID = gs_GraphicsWorld.CreateUIInstance();
    {
        auto& ent = gs_GraphicsWorld.GetUIInstance(uiID);
        ent.entityID = 9999999;
        ent.bindlessGlobalTextureIndex_Albedo = testFont->m_atlasID;
        ent.localToWorld = xform* glm::rotate(glm::radians<float>(180.0f), glm::vec3(0, 1, 0)) ;
        ent.textData = std::move(str);
        ent.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        //ent.fontAsset = testFont.get();
        ent.format.box.max = { 1.0f,1.0f };
        ent.format.box.min = { -1.0f,-1.0f };
        ent.format.fontSize = 0.1f;
        ent.format.alignment = oGFX::FontAlignment::Centre;
        ent.SetText(true);
        ent.flags = UIInstanceFlags(static_cast<uint32_t>(ent.flags) & ~static_cast<uint32_t>(UIInstanceFlags::RENDER_ENABLED));
        ent.flags = UIInstanceFlags(static_cast<uint32_t>(ent.flags) | static_cast<uint32_t>(UIInstanceFlags::RENDER_ENABLED));
    }
    return uiID;
}

void TestApplication::InitDefaultTextures()
{
    uint32_t whiteTexture = 0xFFFFFFFF; // ABGR
    uint32_t blackTexture = 0xFF000000; // ABGR
    uint32_t normalTexture = 0xFFFF8080; // ABGR
    uint32_t pinkTexture = 0xFFA040A0; // ABGR
    
    gs_WhiteTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&whiteTexture));
    gs_BlackTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&blackTexture));
    gs_NormalTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&normalTexture));
    gs_PinkTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&pinkTexture));

    BindlessTextureIndex diffuseTexture0 = gs_RenderEngine->CreateTexture("Textures/7/d.png");
    BindlessTextureIndex diffuseTexture1 = gs_RenderEngine->CreateTexture("Textures/8/d.png");
    BindlessTextureIndex diffuseTexture2 = gs_RenderEngine->CreateTexture("Textures/13/d.png");
    BindlessTextureIndex diffuseTexture3 = gs_RenderEngine->CreateTexture("Textures/23/d.png");

    BindlessTextureIndex roughnessTexture0 = gs_RenderEngine->CreateTexture("Textures/7/r.png");
    BindlessTextureIndex roughnessTexture1 = gs_RenderEngine->CreateTexture("Textures/8/r.png");
    BindlessTextureIndex roughnessTexture2 = gs_RenderEngine->CreateTexture("Textures/13/r.png");
    BindlessTextureIndex roughnessTexture3 = gs_RenderEngine->CreateTexture("Textures/23/r.png");
}

void TestApplication::InitDefaultMeshes()
{
    auto defaultPlaneMesh = CreateDefaultPlaneXZMesh();
    auto defaultCubeMesh = CreateDefaultCubeMesh();
    std::unique_ptr<ModelFileResource> plane{ gs_RenderEngine->LoadMeshFromBuffers(defaultPlaneMesh.m_VertexBuffer, defaultPlaneMesh.m_IndexBuffer, nullptr) };
    std::unique_ptr<ModelFileResource> box{ gs_RenderEngine->LoadMeshFromBuffers(defaultCubeMesh.m_VertexBuffer, defaultCubeMesh.m_IndexBuffer, nullptr) };
}

void TestApplication::LoadMeshTextures(ModelFileResource* model)
{
    for (size_t i = 0; i < model->materials.size(); i++)
    {
        if (model->materials[i].albedo.size()) {
            auto result = gs_RenderEngine->CreateTexture(model->materials[i].albedo);
            model->materials[i].albedoTexture = result;
        }
    }
}

void TestApplication::ProcessModelScene(ModelFileResource* model)
{
    Node* head = model->sceneInfo;

    auto dfs = [&](auto&& dfs, Node* node) ->void{
        if (node) {

            if (node->meshRef != static_cast<uint32_t>(-1)) {
                auto& ed = entities.emplace_back(EntityInfo{});
                ed.name = node->name;
                ed.entityID = FastRandomMagic();
                ed.modelID = gs_test_scene->meshResource;
                ed.scale /= 100.0f;

                uint32_t matID = model->submeshToMaterial[node->meshRef];

                // enable this mesh
                ed.submesh = node->meshRef;

                ed.bindlessGlobalTextureIndex_Albedo = model->materials[matID].albedoTexture;
                ed.bindlessGlobalTextureIndex_Normal = model->materials[matID].normalTexture;
                ed.bindlessGlobalTextureIndex_Metallic = model->materials[matID].albedoTexture;
                ed.bindlessGlobalTextureIndex_Roughness = model->materials[matID].roughnessTexture;
            }

            for (size_t i = 0; i < node->children.size(); i++)
            {
                dfs(dfs, node->children[i]);
            }
        }
    };
    dfs(dfs, head);
   
}

void TestApplication::RunTest_DebugDraw()
{
    if (m_TestDebugDrawLine)
    {
        oGFX::DebugDraw::AddLine(glm::vec3{ 2.0f, 0.0f, 2.0f}, glm::vec3{ 0.0f, 10.0f, 0.0f }, oGFX::Colors::GREEN);
    }

    if (m_TestDebugDrawBox)
	{
		oGFX::AABB aabb;
		aabb.center = { 0.0f,1.0f,0.0f };
		aabb.halfExt = { 1.0f,1.0f,1.0f };
		oGFX::DebugDraw::AddAABB(aabb, oGFX::Colors::GREEN);
	}

	if (m_TestDebugDrawDisc)
	{
        oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 1.0f, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::RED);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 2.0f, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::GREEN);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 1.0f, { 0.0f,1.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::RED);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 2.0f, { 0.0f,1.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::GREEN);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 1.0f, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, oGFX::Colors::RED);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 2.0f, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, oGFX::Colors::GREEN);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 3.0f, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,1.0f }, oGFX::Colors::YELLOW);

		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, -5.0f }, 1.0f, { 0.0f,1.0f,0.0f }, oGFX::Colors::YELLOW);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, -5.0f }, 1.0f, { 0.0f,0.0f,1.0f }, oGFX::Colors::BLUE);
		oGFX::DebugDraw::AddDisc({ 0.0f, 1.0f, -5.0f }, 1.0f, { 1.0f,0.0f,0.0f }, oGFX::Colors::VIOLET);

		oGFX::DebugDraw::AddSphereAs3Disc1HorizonDisc({ -3.0f, 1.0f, -5.0f }, 1.0f, gs_GraphicsWorld.cameras.front().m_position, oGFX::Colors::GREEN);
		oGFX::DebugDraw::AddSphereAs3Disc1HorizonDisc({ -5.0f, 1.0f, -5.0f }, 1.0f, gs_GraphicsWorld.cameras.front().m_position, oGFX::Colors::RED);
	}

    if (m_TestDebugDrawGrid)
    {
        oGFX::DebugDraw::DrawYGrid(100.0f,10.0f);
    }

    if(character_diona)
    {
        oGFX::AABB aabb;
        aabb.halfExt = glm::vec3{ 0.02f };
        // oGFX::Point3D prevpos;

        auto& diona = entities[globalDionaID];
        auto& gfxO = gs_GraphicsWorld.GetObjectInstance(diona.gfxID);
        const auto& refSkeleton = gs_RenderEngine->GetSkeleton(diona.modelID);

        auto* skeleton = diona.localSkeleton;
        int i = 0;



        ImGui::Begin("BONE LA");
       auto DFS = [&](auto&& func, oGFX::BoneNode* pBoneNode, const glm::mat4& parentTransform) -> void
       {

           const glm::mat4 node_local_transform = pBoneNode->mModelSpaceLocal;
           const glm::mat4 node_global_transform = parentTransform * node_local_transform;
           pBoneNode->mModelSpaceGlobal = node_global_transform; // Update global 
           if (pBoneNode->mbIsBoneNode && pBoneNode->m_BoneIndex >= 0)
           {
           ImGui::PushID(i++);
           ImGui::Text(("BONENAME_" + std::to_string(i)).c_str());
           auto& local= pBoneNode->mModelSpaceLocal;
           glm::vec3 xlate;
           glm::vec3 scale;
           glm::vec3 rot;
           ImGuizmo::DecomposeMatrixToComponents(
               glm::value_ptr(local),
               glm::value_ptr(xlate), 
               glm::value_ptr(rot),
               glm::value_ptr(scale));
           ImGui::DragFloat3("trans", glm::value_ptr(xlate));
           ImGui::DragFloat3("rot", glm::value_ptr(rot));
           ImGui::DragFloat3("scale", glm::value_ptr(scale));
           scale.y = scale.z = scale.x; //uniform

           ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(xlate),
               glm::value_ptr(rot),
               glm::value_ptr(scale),
               glm::value_ptr(local));
           ImGui::PopID();


           // If the node isn't a bone then we don't care.
          
               if (gfxO.bones.size())
               {                   
                    gfxO.bones[pBoneNode->m_BoneIndex] = pBoneNode->mModelSpaceGlobal * refSkeleton->inverseBindPose[pBoneNode->m_BoneIndex].transform;
               }
           }
          
          
           ++i;
           // Recursion through all children nodes, passing in the current global transform.
           for (unsigned i = 0; i < pBoneNode->mChildren.size(); i++)
           {
               func(func, pBoneNode->mChildren[i], node_global_transform);
           }
       };
       if (skeleton)
       {
        DFS(DFS, skeleton->m_boneNodes, glm::mat4{ 1.0f });
       }
       ImGui::End();

       if(skeleton)
       {
           auto DrawDFS = [&](auto&& func, oGFX::BoneNode* pBoneNode) -> void
           {
               if (pBoneNode->mChildren.empty())
                   return;

               aabb.center = pBoneNode->mModelSpaceGlobal * glm::vec4(0.0f,0.0f,0.0f,1.0f);
               //oGFX::DebugDraw::AddAABB(aabb, oGFX::Colors::GREEN);

               // Recursion through all children nodes, passing in the current global transform.
               for (unsigned i = 0; i < pBoneNode->mChildren.size(); i++)
               {
                   oGFX::BoneNode* child = pBoneNode->mChildren[i];
                   auto pos = child->mModelSpaceGlobal * glm::vec4(0.0f,0.0f,0.0f,1.0f);
                   //oGFX::DebugDraw::AddLine(aabb.center, pos, oGFX::Colors::RED);
                   func(func, child);
               }
           };
           DrawDFS(DrawDFS, skeleton->m_boneNodes);
       }
    }
    resetBones = false;

    // Debug Draw skeleton
    /*
    if constexpr (false)
    {
        PROFILE_SCOPED("TEST_DrawSkeleton");

        // Update world space global skeleton pose.
        simpleanim::UpdateLocalToGlobalSpace(skinnedMesh_alibaba.get());

        oGFX::AABB aabb;
        aabb.halfExt = { 0.01f,0.01f,0.01f };

        // Trivial and unoptimized method
        auto DFS = [&](auto&& func, simpleanim::BoneNode* pBoneNode) -> void
        {
            if (pBoneNode->mChildren.empty())
                return;

            aabb.center = pBoneNode->mModelSpaceGlobalVqs.GetPosition();
            gs_RenderEngine->AddDebugBox(aabb, oGFX::Colors::GREEN);

            // Recursion through all children nodes, passing in the current global transform.
            for (unsigned i = 0; i < pBoneNode->mChildren.size(); i++)
            {
                simpleanim::BoneNode* child = pBoneNode->mChildren[i].get();
                auto pos = child->mModelSpaceGlobalVqs.GetPosition();
                gs_RenderEngine->AddDebugLine(aabb.center, pos, oGFX::Colors::RED);
                func(func, child);
            }
        };

        //BoneNode* pBoneNode = nullptr;// skinnedMesh_dori->mpRootNode.get();
        simpleanim::BoneNode* pBoneNode = skinnedMesh_alibaba->mpRootNode.get();
        DFS(DFS, pBoneNode);
    }
    */


	// Shamelessly hijack ImGui for debugging tools
	if (m_debugDrawLightPosition)
	{
		if (ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList())
		{
			auto WorldToScreen = [&](const glm::vec3& worldPosition) -> ImVec2
			{
				const int screenWidth = (int)m_WindowSize.x;
				const int screenHeight = (int)m_WindowSize.y;
				const glm::mat4& viewMatrix =gs_GraphicsWorld.cameras.front().matrices.view;
				const glm::mat4& projectionMatrix = gs_GraphicsWorld.cameras.front().GetNonInvProjectionMatrix();
				// World Space to NDC Space
				glm::vec4 ndcPosition = projectionMatrix * viewMatrix * glm::vec4{ worldPosition, 1.0f };
				// Perspective Division
				ndcPosition /= ndcPosition.w;
				// NDC Space to Viewport Space
				const float winX = glm::round(((ndcPosition.x + 1.0f) / 2.0f) * (float)screenWidth);
				const float winY = glm::round(((1.0f - ndcPosition.y) / 2.0f) * (float)screenHeight);
				return ImVec2{ winX, winY };
			};

			for (auto& light : gs_GraphicsWorld.GetAllOmniLightInstances())
			{
				auto& pos = light.position;
				const auto screenPosition = WorldToScreen(pos);
				constexpr float circleSize = 10.0f;
				bgDrawList->AddCircle(ImVec2(screenPosition.x - 0.5f * circleSize, screenPosition.y - 0.5f * circleSize), circleSize, IM_COL32(light.color.r * 0xFF, light.color.g * 0xFF, light.color.b * 0xFF, 0xFF), 0, 2.0f);
			}
		}
	}
}

void TestApplication::ToolUI_Camera()
{
	auto& camera = gs_GraphicsWorld.cameras.front();
	ImGui::DragFloat3("Position", glm::value_ptr(camera.m_position), 0.01f);
	ImGui::DragFloat3("Rotation", glm::value_ptr(camera.m_rotation), 0.01f);
	ImGui::DragFloat3("Target", glm::value_ptr(camera.m_TargetPosition), 0.01f);
	ImGui::DragFloat("Distance", &camera.m_TargetDistance, 0.01f);

    float val = camera.GetNearClip();
    if (ImGui::DragFloat("camnear", &val)) camera.SetNearClip(val);
    val = camera.GetFarClip();
    if (ImGui::DragFloat("cam far", &val)) camera.SetFarClip(val);

    val = camera.GetAspectRatio();
    if (ImGui::DragFloat("ar", &val)) camera.SetAspectRatio(val);

    val = camera.GetFov();
    if (ImGui::DragFloat("fov", &val)) camera.SetFov(val);

    ImGui::BeginDisabled();
    auto proj = camera.matrices.perspective;
    ImGui::DragFloat4("r0", &proj[0][0]);
    ImGui::DragFloat4("r1", &proj[1][0]);
    ImGui::DragFloat4("r2", &proj[2][0]);
    ImGui::DragFloat4("r3", &proj[3][0]);
    ImGui::EndDisabled();

	bool fps = camera.m_CameraMovementType == Camera::CameraMovementType::firstperson;
	if (ImGui::Checkbox("FPS", &fps))
	{
		camera.m_CameraMovementType = fps ? Camera::CameraMovementType::firstperson : Camera::CameraMovementType::lookat;
	}

	ImGui::DragFloat("RotationSpeed", &camera.rotationSpeed);
	ImGui::DragFloat("MovementSpeed", &camera.movementSpeed);

	if (ImGui::Button("Shake"))
	{
		gs_CameraController.ShakeCamera();
	}
}

void TestApplication::ToolUI_Settings()
{
	ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Application");
    ImGui::Text("Luminance: %f", *(float*)gs_RenderEngine->monitorData);
    ImGui::Text("Avg Log Luminance: %f", *((float*)gs_RenderEngine->monitorData+1));
    ImGui::Text("gpu vector bytes : %llu", accumulatedBytes);
	ImGui::Text("m_ApplicationFrame : %u", m_ApplicationFrame);
	ImGui::Text("m_ApplicationTimer : %f", m_ApplicationTimer);
	ImGui::Text("m_ApplicationDT    : %f", m_ApplicationDT);
	ImGui::Checkbox("m_TestDebugDrawLine", &m_TestDebugDrawLine);
	ImGui::Checkbox("m_TestDebugDrawBox", &m_TestDebugDrawBox);
	ImGui::Checkbox("m_TestDebugDrawDisc", &m_TestDebugDrawDisc);
	ImGui::Checkbox("m_TestDebugDrawGrid", &m_TestDebugDrawGrid);
	ImGui::Checkbox("m_TestDebugDrawDecal (not done)", &m_TestDebugDrawDecal);
	ImGui::Separator();
	ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Render Engine");
	ImGui::Checkbox("m_DebugDrawDepthTest", &gs_RenderEngine->m_DebugDrawDepthTest);
	ImGui::Text("g_DebugDrawVertexBufferGPU.size() : %u", gs_RenderEngine->g_DebugDrawVertexBufferGPU.size());
	ImGui::Text("g_DebugDrawIndexBufferGPU.size()  : %u", gs_RenderEngine->g_DebugDrawIndexBufferGPU.size());
	ImGui::Text("g_Textures.size()                 : %u", gs_RenderEngine->g_Textures.size());
	ImGui::Text("g_GlobalMeshBuffers.VtxBuffer.size() : %u", gs_RenderEngine->g_GlobalMeshBuffers.VtxBuffer.size());
	ImGui::Text("g_GlobalMeshBuffers.IdxBuffer.size() : %u", gs_RenderEngine->g_GlobalMeshBuffers.IdxBuffer.size());
	ImGui::Text("g_BoneMatrixBuffers.size() : %u", gs_RenderEngine->gpuBoneMatrixBuffer.size());
	ImGui::Text("boneMatrices.size() : %u", gs_RenderEngine->boneMatrices.size());
	ImGui::Separator();
    {
        ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Scene Settings");
        auto& cs = gs_GraphicsWorld.colourSettings;
        ImGui::DragFloat("exposure", &cs.exposure,0.01f, 0.0f, 12.0f);

        auto& ssaoSettings = gs_RenderEngine->currWorld->ssaoSettings;
        {
            ImGui::Text("SSAO");
            ImGui::PushID(std::atoi("SSAO"));
            ImGui::Checkbox("UseSSAO", &gs_RenderEngine->useSSAO);
            ImGui::DragFloat("Radius", &ssaoSettings.radius, 0.01f);
            ImGui::DragFloat("Bias", &ssaoSettings.bias, 0.01f);
            ImGui::DragFloat("Intensity", &ssaoSettings.intensity, 0.01f);
            uint32_t mmin = 1;
            uint32_t mmax = 64;
            ImGui::DragScalar("Samples", ImGuiDataType_U32, &ssaoSettings.samples, 0.1f, &mmin, &mmax);
            ImGui::PopID();
        }
        
        auto& lightSettings = gs_RenderEngine->currWorld->lightSettings;   
        {
            ImGui::Text("Lighting");
            ImGui::PushID(std::atoi("Lighting"));                      
            ImGui::DragFloat("Ambient", &lightSettings.ambient, 0.01f);
            ImGui::DragFloat3("Directional", &lightSettings.directionalLight.x, 0.01f);
            ImGui::DragFloat4("Directional Color", &lightSettings.directionalLightColor.x, 0.01f);
            ImGui::DragFloat("Max bias", &lightSettings.maxBias, 0.01f);
            ImGui::DragFloat("Bias multiplier", &lightSettings.biasMultiplier, 0.01f);
            ImGui::PopID();
        }       

        if (ImGui::TreeNode("Bloom")) {
            auto& cs = gs_GraphicsWorld.bloomSettings;
            ImGui::Checkbox("Bloom Enabled", &gs_GraphicsWorld.bloomSettings.enabled);
            ImGui::DragFloat("bloom thresh", &cs.threshold, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("soft thresh", &cs.softThreshold, 0.001f, 0.0f, 1.0f);

            ImGui::TreePop();
        }
        
        if (ImGui::TreeNode("ColourCorreciton")) {
            ImGui::DragFloat("shadowThresh", &cs.shadowThreshold, 0.001f, 0.0f, 1.0f);
            ImGui::DragFloat("highThresh", &cs.highlightThreshold, 0.001f, 0.0f, 1.0f);
            ImGui::ColorEdit4("shadow", glm::value_ptr(cs.shadowColour));
            ImGui::ColorEdit4("mid", glm::value_ptr(cs.midtonesColour));
            ImGui::ColorEdit4("high", glm::value_ptr(cs.highlightColour));

            ImGui::TreePop();
        }
        if (ImGui::TreeNode("vignette")) {
            auto& vs = gs_GraphicsWorld.vignetteSettings;
            ImGui::Checkbox("Vignette Enabled", &vs.enabled);
            ImGui::ColorEdit4("vigCol", glm::value_ptr(vs.colour));
            ImGui::DragFloat("innerRadius", &vs.innerRadius, 0.01f);
            ImGui::DragFloat("outerRadius", &vs.outerRadius, 0.01f);

            ImGui::TreePop();
        }

    }
    ImGui::Separator();
    {
		ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Shader Debug Tool");
        if (ImGui::Button("Reload Shaders", {0,40}))
        {
            gs_RenderEngine->m_reloadShaders = true;
        }
		ImGui::DragFloat4("vector4_values0", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values0), 0.01f);      
		ImGui::DragFloat4("vector4_values1", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values1), 0.01f);        
		ImGui::DragFloat4("vector4_values2", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values2), 0.01f);
		ImGui::DragFloat4("vector4_values3", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values3), 0.01f);
		ImGui::DragFloat4("vector4_values4", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values4), 0.01f);
		ImGui::DragFloat4("vector4_values5", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values5), 0.01f);
		ImGui::DragFloat4("vector4_values6", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values6), 0.01f);
		ImGui::DragFloat4("vector4_values7", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values7), 0.01f);
		ImGui::DragFloat4("vector4_values8", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values8), 0.01f);
		ImGui::DragFloat4("vector4_values9", glm::value_ptr(gs_RenderEngine->m_ShaderDebugValues.vector4_values9), 0.01f);
    }
	ImGui::Separator();
    {
		ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Decals");
		ImGui::PushID("TESTDECAL");
		ImGui::DragFloat3("Position", glm::value_ptr(gs_GraphicsWorld.m_HardcodedDecalInstance.position), 0.01f);
		{
			if (ImGui::BeginPopupContextItem("Gizmo hijacker"))
			{
				if (ImGui::Selectable("Set ptr Gizmo"))
				{
					// Shamelessly point to this property (very unsafe, but quick to test shit and speed up iteration time)
					gs_GizmoContext.SelectVector3Property(glm::value_ptr(gs_GraphicsWorld.m_HardcodedDecalInstance.position));
				}
				ImGui::EndPopup();
			}
		}
		ImGui::DragFloat("Projector Size", &gs_GraphicsWorld.m_HardcodedDecalInstance.projectorSize, 0.01f);
		ImGui::DragFloat("nearZ", &gs_GraphicsWorld.m_HardcodedDecalInstance.nearZ, 0.01f);
		ImGui::DragFloat("testVar0", &gs_GraphicsWorld.m_HardcodedDecalInstance.testVar0, 0.01f);
		ImGui::DragFloat("testVar1", &gs_GraphicsWorld.m_HardcodedDecalInstance.testVar1, 0.01f);
		ImGui::DragFloat3("direction", glm::value_ptr(gs_GraphicsWorld.m_HardcodedDecalInstance.direction), 0.01f);
		ImGui::DragFloat("rotation", &gs_GraphicsWorld.m_HardcodedDecalInstance.rotation, 0.5f);
		ImGui::PopID();
    }
}

void TestApplication::TestFrustumCull(oGFX::Frustum f, oGFX::AABB box)
{
    oGFX::Point3D corners[8]{};
    for (int i = 0; i < 8; ++i)
    {
        corners[i] = box.min(); // Start with the minimum corner of the AABB

        if (i & 1) corners[i][0] = box.max()[0];
        if (i & 2) corners[i][1] = box.max()[1];
        if (i & 4) corners[i][2] = box.max()[2];
    }
    auto getplane = [&f = f](size_t i) {
        switch (i)
        {
        case 0:return f.left;
        case 1:return f.right;
        case 2:return f.bottom;
        case 3:return f.top;
        case 4:return f.planeNear;
        case 5:return f.planeFar;
        default: OO_ASSERT(false);
            return oGFX::Plane();
        }};

    auto getppt = [&f = f](size_t i) {
        switch (i)
        {
        case 0:return f.pt_left;
        case 1:return f.pt_right;
        case 2:return f.pt_bottom;
        case 3:return f.pt_top;
        case 4:return f.pt_planeNear;
        case 5:return f.pt_planeFar;
        default: OO_ASSERT(false);
            return oGFX::Point3D();
        }
        };
    for (size_t c = 0; c < 8; c++)
    {
        size_t closest{};
        float maxDist = FLT_MAX;
        for (size_t i = 0; i < 6; i++)
        {
            float t;
            t = oGFX::coll::DistanceToPoint(getplane(i), corners[c]);
            if (std::abs(t) < maxDist) {
                maxDist = std::abs(t);
                closest = i;
            }
        }
        float t;
        t = oGFX::coll::DistanceToPoint(getplane(closest), corners[c]);
        if (t >= 0) {
            oGFX::DebugDraw::AddLine(getppt(closest), corners[c], oGFX::Colors::RED);
        }
        else {
            oGFX::DebugDraw::AddLine(getppt(closest), corners[c], oGFX::Colors::GREEN);
        }
    }
}

void TestApplication::Tool_HandleGizmoManipulation()
{
	if (gs_GizmoContext.AnyPropertySelected())
	{
		PROFILE_SCOPED("Gizmo");

		ImGuizmo::BeginFrame();
		ImGuizmo::Enable(true);
		ImGuizmo::AllowAxisFlip(false);
		ImGuizmo::SetGizmoSizeClipSpace(0.1f);

		static bool prevFrameUsingGizmo = false;
		static bool currFrameUsingGizmo = false;

		// Save the state of the gizmos in the previous frame.
		prevFrameUsingGizmo = currFrameUsingGizmo;
		currFrameUsingGizmo = ImGuizmo::IsUsing();
		const bool firstUseGizmo = currFrameUsingGizmo && !prevFrameUsingGizmo;
		const bool justReleaseGizmo = prevFrameUsingGizmo && !currFrameUsingGizmo;

		ImGuiIO& io = ImGui::GetIO();
		// WR: This is needed if imgui multi-viewport is used, as origin becomes the top left of monitor.
		const auto mainViewportPosition = ImGui::GetMainViewport()->Pos;
		ImGuizmo::SetRect(mainViewportPosition.x, mainViewportPosition.y, io.DisplaySize.x, io.DisplaySize.y);
		glm::mat4x4 viewMatrix = gs_GraphicsWorld.cameras.front().matrices.view;
		glm::mat4x4 projMatrix = gs_GraphicsWorld.cameras.front().GetNonInvProjectionMatrix();

		static glm::mat4x4 localToWorld{ 1.0f };
		float* matrixPtr = glm::value_ptr(localToWorld);

		// Draw cube/grid at the origin for debugging purpose and sanity check
		//ImGuizmo::DrawCubes(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix), matrixPtr, 1);
		//ImGuizmo::DrawGrid(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix), matrixPtr, 20);

		float* gizmoHijack = gs_GizmoContext.GetSelectedVector3Property();
		if (gizmoHijack) // Fix me!
		{
			glm::vec3 position = { gizmoHijack[0], gizmoHijack[1], gizmoHijack[2] };
			localToWorld = glm::translate(glm::mat4{ 1.0f }, position);
			matrixPtr = glm::value_ptr(localToWorld);
		}

		const ImGuizmo::OPERATION gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
		const ImGuizmo::MODE gizmoMode = ImGuizmo::MODE::WORLD;
		const bool gizmoMoved = ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix), gizmoOperation, gizmoMode, matrixPtr);

		if (firstUseGizmo)
		{
			// TODO: What can you do here?
			// - Save the state of the transform? For undo/redo feature?
		}
		else if (justReleaseGizmo)
		{
			// TODO: What can you do here?
			// - Save the state of the transform? For undo/redo feature?
		}

		if (currFrameUsingGizmo)
		{
			if (gizmoMoved)
			{
				// TODO: What can you do here?
				// - Update transform because the gizmo have moved.

				glm::vec3 position, euler_deg, scale; // World Space.
				ImGuizmo::DecomposeMatrixToComponents(matrixPtr, glm::value_ptr(position), glm::value_ptr(euler_deg), glm::value_ptr(scale));
				glm::quat q = glm::quat(glm::radians(euler_deg));

				// Hacky and unsafe...
				gs_GizmoContext.SetVector3Property(position);

				if (EntityInfo* entity = gs_GizmoContext.GetSelectedEntityInfo())
				{
					entity->SyncToGraphicsWorld();
				}

			}
		}
	}
}

void TestApplication::Tool_HandleUI()
{
	if (ImGui::Begin("Scene Helper"))
	{        
        resetBones = ImGui::Button("ResetBones");
		if (ImGui::BeginTabBar("SceneHelperTabBar"))
		{          
			if (ImGui::BeginTabItem("Entity"))
			{
				if (ImGui::SmallButton("Create Cube"))
				{
					auto x = entities.size();
					entities.emplace_back(EntityInfo{});
					auto& entity = entities[x];
					entity.position = { 2.0f,2.0f,2.0f };
					entity.scale = { 1.0f,1.0f,1.0f };
					entity.modelID = gs_ModelID_Box;
					entity.entityID = FastRandomMagic();

					CreateGraphicsEntityHelper(entity);
				}

				int addRandomEntityCount = 0;
				ImGui::Text("Add Random Entity: ");
				if (ImGui::SmallButton("+10")) { addRandomEntityCount = 10; }
				ImGui::SameLine();
				if (ImGui::SmallButton("+50")) { addRandomEntityCount = 50; }
				ImGui::SameLine();
				if (ImGui::SmallButton("+100")) { addRandomEntityCount = 100; }
				ImGui::SameLine();
				if (ImGui::SmallButton("+250")) { addRandomEntityCount = 250; }

				if (addRandomEntityCount)
				{
					for (int i = 0; i < addRandomEntityCount; ++i)
					{
						const glm::vec3 pos = glm::sphericalRand(20.0f);

						auto x = entities.size();
						entities.emplace_back(EntityInfo{});
						auto& entity = entities[x];
						entity.position = { pos.x, glm::abs(pos.y), pos.z };
                        entity.scale = glm::vec3{ 1.0f };
						entity.modelID = gs_ModelID_Box;
						entity.entityID = FastRandomMagic();

						CreateGraphicsEntityHelper(entity);
					}
				}

				ImGui::Text("Total Entities: %u", entities.size());
                if (ImGui::Checkbox("Freeze Spheres", &s_freezeSphere)) {
                    if (s_freezeSphere == true) {
                        ResetSpheres();
                    }
                };
				if (ImGui::TreeNode("Entity"))
				{
                    for (auto& entity : entities)
                    {
                        if (&entity == gs_GizmoContext.GetSelectedEntityInfo()) {
                            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 0, 255));
                        }
                        bool valuesModified = false;
                        ImGui::PushID(entity.entityID);

                        ImGui::BulletText("[ID:%u] ", entity.entityID);
                        ImGui::SameLine();
                        ImGui::Text(entity.name.c_str());
                        if (&entity == gs_GizmoContext.GetSelectedEntityInfo()) {
                            ImGui::PopStyleColor();
                        }
                        auto& msh = gs_RenderEngine->g_globalModels[entity.modelID];
                        ImGui::Text("Submesh");
                        ImGui::Dummy({1, 1});
                        //for (size_t i = 0; i < msh.m_subMeshes.size(); i++)
                        //{
                            ImGui::SameLine();
                            bool val = entity.submesh;
                            ImGui::Text("%d", val);
                        //    if (ImGui::Checkbox(std::to_string(i).c_str(), &val))
                        //    {
                        //        entity.submeshes[i] = val;
                        //        gs_GraphicsWorld.GetObjectInstance(entity.gfxID).submesh = entity.submeshes;
                        //    }
                        //}
						valuesModified |= ImGui::DragFloat3("Position", glm::value_ptr(entity.position), 0.01f);
						{
							if (ImGui::BeginPopupContextItem("Gizmo hijacker"))
							{
								if (ImGui::Selectable("Set ptr Gizmo"))
								{
									gs_GizmoContext.SelectEntityInfo(&entity);
									gs_GizmoContext.SelectVector3Property((float*)&entity.position);
								}
								ImGui::EndPopup();
							}
						}

						valuesModified |= ImGui::DragFloat3("Scale", glm::value_ptr(entity.scale), 0.01f);
						valuesModified |= ImGui::DragFloat3("Rotation Axis", glm::value_ptr(entity.rotVec));
						valuesModified |= ImGui::DragFloat("Theta", &entity.rot);

                        bool renderMe = static_cast<bool>(entity.flags & ObjectInstanceFlags::RENDER_ENABLED);
                        if (ImGui::Checkbox("Renderable", &renderMe))
                        {
                            valuesModified |= true;
                            if (renderMe)
                            {
                                entity.flags = entity.flags | ObjectInstanceFlags::RENDER_ENABLED;
                            }
                            else
                            {
                                auto inv = (~ObjectInstanceFlags::RENDER_ENABLED);
                                entity.flags = entity.flags &inv;
                            }
                        }
						// TODO: We should be using quaternions.........                        
						if (valuesModified)
						{
							entity.SyncToGraphicsWorld();
						}

						ImGui::PopID();


                        if (static_cast<bool>(entity.flags & ObjectInstanceFlags::SKINNED))
                        {
                            if(ImGui::TreeNode("Bones"))
                            {
                                
                                auto& gfx = gs_GraphicsWorld.GetObjectInstance(entity.gfxID);
                                for (size_t i = 0; i < gfx.bones.size(); i++)
                                {
                                    //ImGui::PushID(entity.entityID+i + 1);
                                    //ImGui::Text(("Bones_" + std::to_string(i)).c_str());
                                    //{                                    
                                    //    glm::vec3 scale;
                                    //    glm::vec3 rot;
                                    //    glm::vec3 xlate;
                                    //    ImGuizmo::DecomposeMatrixToComponents(
                                    //        glm::value_ptr(gfx.bones[i]),
                                    //        glm::value_ptr(xlate), 
                                    //        glm::value_ptr(rot),
                                    //        glm::value_ptr(scale));
                                    //    ImGui::DragFloat3("trans", glm::value_ptr(xlate));
                                    //    ImGui::DragFloat3("rot", glm::value_ptr(rot));
                                    //    ImGui::DragFloat3("scale", glm::value_ptr(scale));
                                    //    scale.y = scale.z = scale.x; //uniform
                                    //    
                                    //    ImGuizmo::RecomposeMatrixFromComponents(glm::value_ptr(xlate),
                                    //        glm::value_ptr(rot),
                                    //        glm::value_ptr(scale),
                                    //        glm::value_ptr(gfx.bones[i]));
                                    //    
                                    //ImGui::PopID();
                                    //}
                                }
                                ImGui::TreePop();
                            }

                        }

					}

					ImGui::TreePop();
				}//ImGui::TreeNode

				ImGui::EndTabItem();
			}//ImGui::BeginTabItem

			if (ImGui::BeginTabItem("Light"))
			{
                ImGui::Checkbox("Draw Debug lights", &drawDebugLights);
				if (ImGui::SmallButton("Create PointLight")) 
                {
                    int32_t lightId = gs_GraphicsWorld.CreateLightInstance();
                    auto& newLight = gs_GraphicsWorld.GetLightInstance(lightId);
                    newLight.position = {};
                    newLight.radius.x = 3.0f;
                    newLight.color = { 1.0f,1.0f,1.0f,1.0f };

                }

				ImGui::Checkbox("Freeze Lights", &s_freezeLight);
                if (ImGui::Button("Off lights"))
                {
                    for (size_t i = 0; i < hardCodedLights; i++)
                    {
                        auto& l = gs_GraphicsWorld.GetLightInstance((int32_t)i);
                        l.radius.x = 0.0f;
                    }
                }
				ImGui::Checkbox("Debug Draw Position", &m_debugDrawLightPosition);
				ImGui::Separator();
				
				{
                    int i = 0;
                    for (auto& light : gs_RenderEngine->currWorld->GetAllOmniLightInstances())
                    {
                        if (drawDebugLights == true) {
                            oGFX::Sphere s(light.position, light.radius.x);
                            oGFX::DebugDraw::AddSphere(s, light.color);
                        }
					    ImGui::PushID(i++);
                        SetLightEnabled(light,true);
                        ImGui::DragFloat3("Position", glm::value_ptr(light.position), 0.01f);
                        {
                            if (ImGui::BeginPopupContextItem("Gizmo hijacker"))
                            {
                                if (ImGui::Selectable("Set ptr Gizmo"))
                                {
                                    // Shamelessly point to this property (very unsafe, but quick to test shit and speed up iteration time)
                                    gs_GizmoContext.SelectVector3Property(glm::value_ptr(light.position));
                                }
                                ImGui::EndPopup();
                            }
                        }
                       

                        int light_type_selector = 0;
                        light_type_selector = light.info.w -1;
                        static const char* lights_types[]{
                            //"OFF",
                            "POINT",
                            "AREA",
                        };
                        if (ImGui::ListBox("LightType", &light_type_selector, lights_types, 2))
                        {
                            light.info.w = light_type_selector + 1;
                        }
                        ImGui::DragFloat3("Color", glm::value_ptr(light.color),0.1f,0.0f,1.0f);
                        ImGui::DragFloat("Intensity", &light.color.a);
                        ImGui::DragFloat("Radius", &light.radius.x,0.1f,0.0f);
                        ImGui::DragFloat3("Direction", glm::value_ptr(light.view[0][0]));
                        {
                            bool sh = GetCastsShadows(light);
                            if (ImGui::Checkbox("Shadows", &sh))
                            {
                                SetCastsShadows(light, sh);
                            }
                        }
                        ImGui::PopID();
                    }				
				}

				ImGui::EndTabItem();
			}//ImGui::BeginTabItem

			if (ImGui::BeginTabItem("Camera"))
			{
				ToolUI_Camera();

				ImGui::EndTabItem();
			}//ImGui::BeginTabItem

			if (ImGui::BeginTabItem("Settings"))
			{
				ToolUI_Settings();

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}//ImGui::BeginTabBar
	}//ImGui::Begin

	ImGui::End();
}

void InitLights(int32_t* someLights)
{    
    for (size_t i = 0; i < hardCodedLights; i++)
    {
        someLights[i] = gs_GraphicsWorld.CreateLightInstance();
    }
    {
        OmniLightInstance* lights[hardCodedLights];
        for (size_t i = 0; i < hardCodedLights; i++)
        {
            lights[i] = &gs_GraphicsWorld.GetLightInstance(someLights[i]);
            SetLightEnabled(*lights[i], true);
        }
        // put here so we can edit the light values
        lights[0]->position = glm::vec4(0.0f, 3.0f, 1.0f, 0.0f);
        lights[0]->color = glm::vec4(1.0f);
        lights[0]->radius.x = 30.0f;
        lights[0]->color.a = 90.0f; //intensity
        SetCastsShadows(*lights[0], true);
        SetLightEnabled(*lights[0], true);

        return;
        // Red   
        lights[1]->position = glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f);
        lights[1]->color = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        lights[1]->radius.x = 15.0f;
        // Blue  
        lights[2]->position = glm::vec4(2.0f, -1.0f, 0.0f, 0.0f);
        lights[2]->color = glm::vec4(0.0f, 0.0f, 2.5f, 0.0f);
        lights[2]->radius.x = 5.0f;
        // Yellow
        lights[3]->position = glm::vec4(0.0f, -0.9f, 0.5f, 0.0f);
        lights[3]->color = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
        lights[3]->radius.x = 2.0f;
        // Green 
        lights[4]->position = glm::vec4(0.0f, -0.5f, 0.0f, 0.0f);
        lights[4]->color = glm::vec4(0.0f, 1.0f, 0.2f, 0.0f);
        lights[4]->radius.x = 5.0f;
        // Yellow
        lights[5]->position = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
        lights[5]->color = glm::vec4(1.0f, 0.7f, 0.3f, 0.0f);
        lights[5]->radius.x = 25.0f;
    }
}
