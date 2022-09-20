﻿#include "TestApplication.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#if defined(_WIN32)
#define NOMINMAX
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

#include "ImGuizmo.h"
#include "AppUtils.h"

#include "CameraController.h"
#include "DebugDraw.h"

//#include "anim/SimpleAnim.h" // WR ONLY

static VulkanRenderer* gs_RenderEngine = nullptr;
static GraphicsWorld gs_GraphicsWorld;

using BindlessTextureIndex = uint32_t;
static constexpr BindlessTextureIndex INVALID_BINDLESS_TEXTURE_INDEX = 0xFFFFFFFF;

static BindlessTextureIndex gs_WhiteTexture  = INVALID_BINDLESS_TEXTURE_INDEX;
static BindlessTextureIndex gs_BlackTexture  = INVALID_BINDLESS_TEXTURE_INDEX;
static BindlessTextureIndex gs_NormalTexture = INVALID_BINDLESS_TEXTURE_INDEX;
static BindlessTextureIndex gs_PinkTexture   = INVALID_BINDLESS_TEXTURE_INDEX;

struct DummyTestMaterial
{
    BindlessTextureIndex albedo;
    BindlessTextureIndex normal;
    BindlessTextureIndex roughness;
    BindlessTextureIndex metallic;
};

void TestApplication::Init()
{

}

// this is to pretend we have an ECS system
struct EntityInfo
{
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
    uint8_t instanceData{ 0 };

    int32_t gfxID; // gfxworld id

    mat4 getLocalToWorld()
    {
        glm::mat4 xform{ 1.0f };
        xform = glm::translate(xform, position);
        xform = glm::rotate(xform, glm::radians(rot), rotVec);
        xform = glm::scale(xform, scale);
        return xform;
    }

    void SyncToGraphicsWorld()
    {
        auto& gfxWorldObjectInstance = gs_GraphicsWorld.GetObjectInstance(gfxID);
        gfxWorldObjectInstance.position = position;
        gfxWorldObjectInstance.scale = scale;
        gfxWorldObjectInstance.rot = rot;
        gfxWorldObjectInstance.rotVec = rotVec;
        gfxWorldObjectInstance.localToWorld = getLocalToWorld();
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
    AABB ab;
    auto& model = gs_RenderEngine->models[ei.modelID];

    //UpdateBV(gs_RenderEngine->models[e.modelID].cpuModel, e);

    auto id = gs_GraphicsWorld.CreateObjectInstance(
        // Very unsafe to use initializer list, prone to mistakes...
        ObjectInstance{
            ei.name,
            ei.position,
            ei.scale,
            ei.rot,
            ei.rotVec,
            ei.bindlessGlobalTextureIndex_Albedo,
            ei.bindlessGlobalTextureIndex_Normal,
            ei.bindlessGlobalTextureIndex_Roughness,
            ei.bindlessGlobalTextureIndex_Metallic,
            ei.instanceData,
            ei.getLocalToWorld(),
            {},
            ei.modelID,
            ei.entityID
        }
    );
    // assign id
    ei.gfxID = id;
}

static std::vector<EntityInfo> entities;

static CameraController gs_CameraController;

static GizmoContext gs_GizmoContext{};

void TestApplication::Run()
{
    gs_RenderEngine = VulkanRenderer::get();

    //----------------------------------------------------------------------------------------------------
    // Setup App Window
    //----------------------------------------------------------------------------------------------------

    AppWindowSizeTypes appWindowSizeType = AppWindowSizeTypes::HD_900P_16_10;
    const glm::ivec2 windowSize = gs_AppWindowSizes[(int)appWindowSizeType];
    Window mainWindow(windowSize.x, windowSize.y);
    mainWindow.Init();

    oGFX::SetupInfo setupSpec;

    //setupSpec.debug = BoolQueryUser("Do you want debugging?");
    //setupSpec.renderDoc = BoolQueryUser("Do you want renderdoc?");
    setupSpec.debug = true;
    setupSpec.renderDoc = true;

    //----------------------------------------------------------------------------------------------------
    // Setup Graphics Engine
    //----------------------------------------------------------------------------------------------------

    try
    {
        gs_RenderEngine->Init(setupSpec, mainWindow);

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
    }
    catch (const std::exception& e)
    {
        std::cout << "Failed to create Vulkan instance!" << std::endl;
        std::cout << e.what() << std::endl;
        getchar();
        return;
    }

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Textures
    //----------------------------------------------------------------------------------------------------

    uint32_t whiteTexture = 0xFFFFFFFF; // ABGR
    uint32_t blackTexture = 0xFF000000; // ABGR
    uint32_t normalTexture = 0xFFFF8080; // ABGR
    uint32_t pinkTexture = 0xFFA040A0; // ABGR

    gs_WhiteTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&whiteTexture));
    gs_BlackTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&blackTexture));
    gs_NormalTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&normalTexture));
    gs_PinkTexture = gs_RenderEngine->CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&pinkTexture));

    const BindlessTextureIndex diffuseTexture0 = gs_RenderEngine->CreateTexture("Textures/7/d.png");
    const BindlessTextureIndex diffuseTexture1 = gs_RenderEngine->CreateTexture("Textures/8/d.png");
    const BindlessTextureIndex diffuseTexture2 = gs_RenderEngine->CreateTexture("Textures/13/d.png");
    const BindlessTextureIndex diffuseTexture3 = gs_RenderEngine->CreateTexture("Textures/23/d.png");

    const BindlessTextureIndex roughnessTexture0 = gs_RenderEngine->CreateTexture("Textures/7/r.png");
    const BindlessTextureIndex roughnessTexture1 = gs_RenderEngine->CreateTexture("Textures/8/r.png");
    const BindlessTextureIndex roughnessTexture2 = gs_RenderEngine->CreateTexture("Textures/13/r.png");
    const BindlessTextureIndex roughnessTexture3 = gs_RenderEngine->CreateTexture("Textures/23/r.png");

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Models
    //----------------------------------------------------------------------------------------------------

    auto defaultPlaneMesh = CreateDefaultPlaneXZMesh();
    auto defaultCubeMesh = CreateDefaultCubeMesh();
    std::unique_ptr<ModelData> model_plane{ gs_RenderEngine->LoadMeshFromBuffers(defaultPlaneMesh.m_VertexBuffer, defaultPlaneMesh.m_IndexBuffer, nullptr) };
    std::unique_ptr<ModelData> model_box{ gs_RenderEngine->LoadMeshFromBuffers(defaultCubeMesh.m_VertexBuffer, defaultCubeMesh.m_IndexBuffer, nullptr) };

    std::unique_ptr<ModelData> character_diona{ gs_RenderEngine->LoadModelFromFile("../Application/models/character/diona.fbx") };
    std::unique_ptr<ModelData> character_qiqi{ gs_RenderEngine->LoadModelFromFile("../Application/models/character/qiqi.fbx") };

    /* // WIP
    std::unique_ptr<ModelData> alibaba{ gs_RenderEngine->LoadModelFromFile("../Application/models/anim/AnimationTest_Box.fbx") };
    std::unique_ptr<simpleanim::SkinnedMesh> skinnedMesh_alibaba = std::make_unique<simpleanim::SkinnedMesh>();
    simpleanim::LoadModelFromFile_Skeleton("../Application/models/anim/AnimationTest_Box.fbx", simpleanim::LoadingConfig{}, alibaba.get(), skinnedMesh_alibaba.get());
    */
    /* // WIP
    std::unique_ptr<ModelData> character_dori{ gs_RenderEngine->LoadModelFromFile("../Application/models/character/dori.fbx") };
    std::unique_ptr<SkinnedMesh> skinnedMesh_dori = std::make_unique<SkinnedMesh>();
    LoadModelFromFile_Skeleton("../Application/models/character/dori.fbx", LoadingConfig{}, character_dori.get(), skinnedMesh_dori.get());
    */
    //----------------------------------------------------------------------------------------------------
    // Setup Initial Scene Objects
    //----------------------------------------------------------------------------------------------------

    // Comment/Uncomment as needed
    std::unique_ptr<ModelData> test_scene{ gs_RenderEngine->LoadModelFromFile("../Application/models/testScene.fbx") };
    //std::unique_ptr<ModelData> test_scene = nullptr;

    std::array<uint32_t, 4> diffuseBindlessTextureIndexes =
    {
        diffuseTexture0, diffuseTexture1, diffuseTexture2, diffuseTexture3
    };

    std::array<uint32_t, 4> roughnessBindlessTextureIndexes =
    {
        roughnessTexture0, roughnessTexture1, roughnessTexture2, roughnessTexture3
    };

    // Temporary solution to assign random numbers... Shamelessly stolen from Intel.
    auto FastRandomMagic = []() -> uint32_t
    {
        static uint32_t seed = 0xDEADBEEF;
        seed = (214013 * seed + 2531011);
        return (seed >> 16) & 0x7FFF;
    };

    {
        auto& ed = entities.emplace_back(EntityInfo{});
        ed.name = "Ground_Plane";
        ed.entityID = FastRandomMagic();
        ed.modelID = model_plane->gfxMeshIndices.front();
        ed.position = { 0.0f,0.0f,0.0f };
        ed.scale = { 15.0f,1.0f,15.0f };
    }

    {
        auto& ed = entities.emplace_back(EntityInfo{});
        ed.name = "Plane_Effects";
        ed.entityID = FastRandomMagic();
        ed.modelID = model_plane->gfxMeshIndices.front();
        ed.position = { -2.0f,2.0f,0.0f };
        ed.scale = { 2.0f,1.0f,2.0f };
        ed.rot = 90.0f;
        ed.rotVec = { 1.0f,0.0f,0.0f };
        ed.instanceData = 3;
    }


    if (test_scene)
    {
        auto& ed = entities.emplace_back(EntityInfo{});
        ed.name = "TestSceneObject";
        ed.entityID = FastRandomMagic();
        ed.modelID = test_scene->gfxMeshIndices.front();
        ed.position = {  };
        ed.scale = { 0.1f,0.1f,0.1f };
        ed.rot = 0.0f;
        ed.rotVec = { 1.0f,0.0f,0.0f };
        ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        ed.instanceData = 0;
    }

    // Create 8 more surrounding planes
    {
        for (int i = 0; i < 8; ++i)
        {
            constexpr float offset = 16.0f;
            static std::array<glm::vec3, 8> positions =
            {
                glm::vec3{  offset, 0.0f,   0.0f },
                glm::vec3{ -offset, 0.0f,   0.0f },
                glm::vec3{    0.0f, 0.0f,  offset },
                glm::vec3{    0.0f, 0.0f, -offset },
                glm::vec3{  offset, 0.0f,  offset },
                glm::vec3{ -offset, 0.0f,  offset },
                glm::vec3{  offset, 0.0f, -offset },
                glm::vec3{ -offset, 0.0f, -offset },
            };

            auto& ed = entities.emplace_back(EntityInfo{});
            ed.name = "Ground_Plane_" + std::to_string(i);
            ed.entityID = FastRandomMagic();
            ed.modelID = model_plane->gfxMeshIndices.front();
            ed.position = positions[i];
            ed.scale = { 15.0f,1.0f,15.0f };
            ed.bindlessGlobalTextureIndex_Albedo = diffuseBindlessTextureIndexes[i / 2];
            ed.bindlessGlobalTextureIndex_Roughness = roughnessBindlessTextureIndexes[i / 2];
        }
    }

    std::function<void(ModelData*,Node*)> EntityHelper = [&](ModelData* model,Node* node) {
        if (node->meshRef != static_cast<uint32_t>(-1))
        {
            auto& ed = entities.emplace_back(EntityInfo{});
            ed.modelID = model->gfxMeshIndices[node->meshRef];
            ed.name = node->name;
            ed.entityID = FastRandomMagic();
            glm::quat qt;
            glm::vec3 skew;
            glm::vec4 persp;
            ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(node->transform), glm::value_ptr(ed.position), glm::value_ptr(ed.rotVec), glm::value_ptr(ed.scale));
            //glm::decompose(node->transform, ed.scale, qt, ed.position, skew, persp);
            //qt = glm::conjugate(qt);
            ////glm::angle
            //ed.rotVec = glm::eulerAngles(qt);
            ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
            ed.instanceData = 0;
        }
        for (auto& child : node->children)
        {
            EntityHelper(model,child);
        }            

    };
    if (character_diona)
    {
        
        EntityHelper(character_diona.get(),character_diona->sceneInfo);
        //auto& ed = entities.emplace_back(EntityInfo{});
        //ed.modelID = character_diona->gfxMeshIndices.front();
        //ed.name = "diona";
        //ed.entityID = FastRandomMagic();
        //ed.position = { 2.0f,0.0f,0.0f };
        //ed.scale = { 1.0f,1.0f,1.0f };
        //ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        //ed.instanceData = 1;
    }

    if (character_qiqi)
    {
        EntityHelper(character_qiqi.get(), character_qiqi->sceneInfo);
        //auto& ed = entities.emplace_back(EntityInfo{});
        //ed.modelID = character_qiqi->gfxMeshIndices.front();
        //ed.name = "qiqi";
        //ed.entityID = FastRandomMagic();
        //ed.position = { 1.0f,0.0f,0.0f };
        //ed.scale = { 1.0f,1.0f,1.0f };
        //ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        //ed.instanceData = 2;
    }

    /* // WIP
    if (alibaba)
    {
        auto& ed = entities.emplace_back(EntityInfo{});
        ed.modelID = alibaba->gfxMeshIndices.front();
        ed.name = "alibaba";
        ed.entityID = FastRandomMagic();
        ed.position = { 0.0f,0.0f,0.0f };
        ed.scale = { 1.0f,1.0f,1.0f };
        ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        //ed.instanceData = 2;
    }
    */
    
    if (test_scene)
    {
        EntityHelper(test_scene.get(), test_scene->sceneInfo);
    }

    // Stress test more models
    std::vector<std::unique_ptr<ModelData>> moreModels;
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
            auto& ed = entities.emplace_back(EntityInfo{});
            ed.modelID = model->gfxMeshIndices.front();
            ed.name = "model_" + std::to_string(counter);
            ed.entityID = FastRandomMagic();
            ed.position = { 2.0f + 2.0 * float(counter % 4), 0.0f, -(2.0f + 2.0 * float(counter / 4)) };
            ed.scale = { 0.01f,0.01f,0.01f };
            ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
            ++counter;
        }
    }

    // Transfer to Graphics World
    for (auto& e : entities)
    {
        CreateGraphicsEntityHelper(e);
    }

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Camera
    //----------------------------------------------------------------------------------------------------
    {
        auto& camera = gs_RenderEngine->camera;
        gs_CameraController.SetCamera(&camera);

        camera.m_CameraMovementType = Camera::CameraMovementType::firstperson;
        camera.movementSpeed = 5.0f;
        
        camera.SetRotation(glm::vec3(0.0f, 180.0f, 0.0f));
        camera.SetRotationSpeed(0.5f);
        camera.SetPosition(glm::vec3(0.0f, 2.0f, 4.0f));
        camera.SetAspectRatio((float)mainWindow.m_width / (float)mainWindow.m_height);
    }

    auto lastTime = std::chrono::high_resolution_clock::now();
    static bool freezeLight = true;

    //----------------------------------------------------------------------------------------------------
    // Set graphics world before rendering
    //----------------------------------------------------------------------------------------------------
    gs_RenderEngine->SetWorld(&gs_GraphicsWorld);

    gs_GraphicsWorld.m_HardcodedDecalInstance.position = glm::vec3{ 0.0f,0.0f,0.0f };

    //----------------------------------------------------------------------------------------------------
    // Application Loop
    //----------------------------------------------------------------------------------------------------
    {
        while (mainWindow.windowShouldClose == false)  // infinite loop
        {
            PROFILE_FRAME("MainThread");

            auto now = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(now - lastTime).count();
            lastTime = now;

            gs_RenderEngine->renderClock += deltaTime;

            //reset keys
            Input::Begin();
            while (Window::PollEvents());

            //------------------------------
            // Camera Controller
            //------------------------------
            // Decoupling camera logic.
            // Graphics should only know the final state before calculating the view & projection matrix.
            {
                auto& camera = gs_RenderEngine->camera;

                gs_CameraController.SetCamera(&camera);
                // If the aspect ratio changes, then the projection matrix must be updated correctly...
                camera.SetAspectRatio((float)mainWindow.m_width / (float)mainWindow.m_height);
                gs_CameraController.Update(deltaTime);
            }

            if (Input::GetKeyTriggered(KEY_SPACE))
            {
                freezeLight = !freezeLight;
            }

            {
                PROFILE_SCOPED("ImGui::NewFrame");
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
            }

            if (gs_RenderEngine->PrepareFrame() == true)
            {
                PROFILE_SCOPED("gs_RenderEngine->PrepareFrame() == true");

                if (freezeLight == false)
                {
                    {
                        auto& lights = gs_GraphicsWorld.m_HardcodedOmniLights;

                        static float lightTimer = 0.0f;
                        lightTimer += deltaTime * 0.25f;

                        lights[0].position = glm::vec4(0.0f, 0.0f, 1.0f, 0.0f);
                        lights[0].color = glm::vec4(1.5f);
                        lights[0].radius.x = 15.0f;
                        // Red
                        lights[1].position = glm::vec4(-2.0f, 0.0f, 0.0f, 0.0f);
                        lights[1].color = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
                        lights[1].radius.x = 15.0f;
                        // Blue
                        lights[2].position = glm::vec4(2.0f, -1.0f, 0.0f, 0.0f);
                        lights[2].color = glm::vec4(0.0f, 0.0f, 2.5f, 0.0f);
                        lights[2].radius.x = 5.0f;
                        // Yellow
                        lights[3].position = glm::vec4(0.0f, -0.9f, 0.5f, 0.0f);
                        lights[3].color = glm::vec4(1.0f, 1.0f, 0.0f, 0.0f);
                        lights[3].radius.x = 2.0f;
                        // Green
                        lights[4].position = glm::vec4(0.0f, -0.5f, 0.0f, 0.0f);
                        lights[4].color = glm::vec4(0.0f, 1.0f, 0.2f, 0.0f);
                        lights[4].radius.x = 5.0f;
                        // Yellow
                        lights[5].position = glm::vec4(0.0f, -1.0f, 0.0f, 0.0f);
                        lights[5].color = glm::vec4(1.0f, 0.7f, 0.3f, 0.0f);
                        lights[5].radius.x = 25.0f;

                        lights[0].position.x = sin(glm::radians(360.0f * lightTimer)) * 5.0f;
                        lights[0].position.z = cos(glm::radians(360.0f * lightTimer)) * 5.0f;

                        lights[1].position.x = -4.0f + sin(glm::radians(360.0f * lightTimer) + 45.0f) * 2.0f;
                        lights[1].position.z = 0.0f + cos(glm::radians(360.0f * lightTimer) + 45.0f) * 2.0f;

                        lights[2].position.x = 4.0f + sin(glm::radians(360.0f * lightTimer)) * 2.0f;
                        lights[2].position.z = 0.0f + cos(glm::radians(360.0f * lightTimer)) * 2.0f;

                        lights[4].position.x = 0.0f + sin(glm::radians(360.0f * lightTimer + 90.0f)) * 5.0f;
                        lights[4].position.z = 0.0f - cos(glm::radians(360.0f * lightTimer + 45.0f)) * 5.0f;

                        lights[5].position.x = 0.0f + sin(glm::radians(-360.0f * lightTimer + 135.0f)) * 10.0f;
                        lights[5].position.z = 0.0f - cos(glm::radians(-360.0f * lightTimer - 45.0f)) * 10.0f;
                    }
                }

                // Upload CPU light data to GPU. Ideally this should only contain lights that intersects the camera frustum.
                gs_RenderEngine->UploadLights();

                //gs_RenderEngine->AddDebugLine(glm::vec3{ 0.0f, 0.0f, 0.0f}, glm::vec3{ 2.0f, 2.0f, 2.0f }, oGFX::Colors::GREEN);

                // We need to test debug draw...
                if (m_TestDebugDrawBox)
                {
                    AABB aabb;
                    aabb.center = { 0.0f,1.0f,0.0f };
                    aabb.halfExt = { 1.0f,1.0f,1.0f };
                    DebugDraw::AddAABB(aabb, oGFX::Colors::GREEN);
                }
                if (m_TestDebugDrawDisc)
                {
                    DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 1.0f, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::RED);
                    DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 2.0f, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::GREEN);
                    DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 1.0f, { 0.0f,1.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::RED);
                    DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 2.0f, { 0.0f,1.0f,0.0f }, { 0.0f,0.0f,1.0f }, oGFX::Colors::GREEN);
					DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 1.0f, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, oGFX::Colors::RED);
					DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 2.0f, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, oGFX::Colors::GREEN);
					DebugDraw::AddDisc({ 0.0f, 1.0f, 0.0f }, 3.0f, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,1.0f }, oGFX::Colors::YELLOW);
                }


                // Debug Draw skeleton
                /*
                if constexpr (false)
                {
                    PROFILE_SCOPED("TEST_DrawSkeleton");

                    // Update world space global skeleton pose.
                    simpleanim::UpdateLocalToGlobalSpace(skinnedMesh_alibaba.get());
                    
                    AABB aabb;
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

                // Render the frame
                gs_RenderEngine->RenderFrame();

                // Create a dockspace over the mainviewport so that we can dock stuff
                ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
                    ImGuiDockNodeFlags_PassthruCentralNode // make the dockspace transparent
                    | ImGuiDockNodeFlags_NoDockingInCentralNode // dont allow docking in the central area
                );

                //ImGui::ShowDemoWindow();

                // ImGuizmo
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
                    glm::mat4x4 viewMatrix = gs_RenderEngine->camera.matrices.view;
                    glm::mat4x4 projMatrix = gs_RenderEngine->camera.matrices.perspective;

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
                // Display ImGui Window
                {
                    PROFILE_SCOPED("ImGuiSceneHelper");
                    if (ImGui::Begin("Scene Helper"))
                    {                       
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
                                    entity.modelID = model_box->gfxMeshIndices.front();
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
                                        entity.scale = { 1.0f,1.0f,1.0f };
                                        entity.modelID = model_box->gfxMeshIndices.front();
                                        entity.entityID = FastRandomMagic();
                                        
                                        CreateGraphicsEntityHelper(entity);
                                    }
                                }

                                ImGui::Text("Total Entities: %u", entities.size());

                                if (ImGui::TreeNode("Entity"))
                                {
                                    for (auto& entity : entities)
                                    {
                                        bool valuesModified = false;
                                        ImGui::PushID(entity.entityID);

                                        ImGui::BulletText("[ID:%u] ", entity.entityID);
                                        ImGui::SameLine();
                                        ImGui::Text(entity.name.c_str());
                                        valuesModified |= ImGui::DragFloat3("Position", glm::value_ptr(entity.position), 0.01f);
                                        {
                                            if (ImGui::BeginPopupContextItem("Gizmo hijacker"))
                                            {
                                                if (ImGui::Selectable("Set ptr Gizmo"))
                                                {
                                                    gs_GizmoContext.SelectEntityInfo(&entity);
                                                    gs_GizmoContext.SelectVector3Property((float*)& entity.position);
                                                }
                                                ImGui::EndPopup();
                                            }
                                        }

                                        valuesModified |= ImGui::DragFloat3("Scale", glm::value_ptr(entity.scale), 0.01f);
                                        valuesModified |= ImGui::DragFloat3("Rotation Axis", glm::value_ptr(entity.rotVec));
                                        valuesModified |= ImGui::DragFloat("Theta", &entity.rot);
                                        // TODO: We should be using quaternions.........

                                        if (valuesModified)
                                        {
                                            entity.SyncToGraphicsWorld();
                                        }

                                        ImGui::PopID();
                                    }

                                    ImGui::TreePop();
                                }//ImGui::TreeNode

                                ImGui::EndTabItem();
                            }//ImGui::BeginTabItem

                            if (ImGui::BeginTabItem("Light"))
                            {
                                ImGui::BeginDisabled(); // TODO remove once implemented
                                if (ImGui::SmallButton("Create PointLight")) {} // TODO Implement me!
                                ImGui::EndDisabled(); // TODO remove once implemented

                                static bool debugDrawPosition = false;
                                ImGui::Checkbox("Freeze Lights", &freezeLight);
                                ImGui::Checkbox("Debug Draw Position", &debugDrawPosition);
                                ImGui::Separator();
                                for (int i = 0; i < 6; ++i)
                                {
                                    ImGui::PushID(i);
                                    auto& light = gs_RenderEngine->currWorld->m_HardcodedOmniLights[i];
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

                                    ImGui::DragFloat3("Color", glm::value_ptr(light.color));
                                    ImGui::DragFloat("Radius", &light.radius.x);
                                    ImGui::PopID();
                                }

                                // Shamelessly hijack ImGui for debugging tools
                                if (debugDrawPosition)
                                {
                                    if (ImDrawList* bgDrawList = ImGui::GetBackgroundDrawList())
                                    {
                                        auto WorldToScreen = [&](const glm::vec3& worldPosition) -> ImVec2
                                        {
                                            const int screenWidth = (int)windowSize.x;
                                            const int screenHeight = (int)windowSize.y;
                                            const glm::mat4& viewMatrix = gs_RenderEngine->camera.matrices.view;
                                            const glm::mat4& projectionMatrix = gs_RenderEngine->camera.matrices.perspective;
                                            // World Space to NDC Space
                                            glm::vec4 ndcPosition = projectionMatrix * viewMatrix * glm::vec4{ worldPosition, 1.0f };
                                            // Perspective Division
                                            ndcPosition /= ndcPosition.w;
                                            // NDC Space to Viewport Space
                                            const float winX = glm::round(((ndcPosition.x + 1.0f) / 2.0f) * (float)screenWidth);
                                            const float winY = glm::round(((1.0f - ndcPosition.y) / 2.0f) * (float)screenHeight);
                                            return ImVec2{ winX, winY };
                                        };

                                        for (int i = 0; i < 6; ++i)
                                        {
                                            auto& light = gs_RenderEngine->currWorld->m_HardcodedOmniLights[i];
                                            auto& pos = light.position;
                                            const auto screenPosition = WorldToScreen(pos);
                                            constexpr float circleSize = 10.0f;
                                            bgDrawList->AddCircle(ImVec2(screenPosition.x - 0.5f * circleSize, screenPosition.y - 0.5f * circleSize), circleSize, IM_COL32(light.color.r * 0xFF, light.color.g * 0xFF, light.color.b * 0xFF, 0xFF), 0, 2.0f);
                                        }
                                    }
                                }

                                ImGui::EndTabItem();
                            }//ImGui::BeginTabItem

                            if (ImGui::BeginTabItem("Camera"))
                            {
                                auto& camera = gs_RenderEngine->camera;
                                ImGui::DragFloat3("Position", glm::value_ptr(camera.m_position), 0.01f);
                                ImGui::DragFloat3("Rotation", glm::value_ptr(camera.m_rotation), 0.01f);
                                ImGui::DragFloat3("Target", glm::value_ptr(camera.m_TargetPosition), 0.01f);
                                ImGui::DragFloat("Distance", &camera.m_TargetDistance, 0.01f);

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

                                ImGui::EndTabItem();
                            }//ImGui::BeginTabItem

                            if (ImGui::BeginTabItem("Settings"))
                            {
                                ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Application");
                                ImGui::Checkbox("m_TestDebugDrawBox", &m_TestDebugDrawBox);
                                ImGui::Checkbox("m_TestDebugDrawDisc", &m_TestDebugDrawDisc);
                                ImGui::Checkbox("m_TestDebugDrawDecal (not done)", &m_TestDebugDrawDecal);
                                ImGui::Separator();
                                ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Render Engine");
                                ImGui::Checkbox("m_DebugDrawDepthTest", &gs_RenderEngine->m_DebugDrawDepthTest);
                                ImGui::Text("g_DebugDrawVertexBufferGPU.size() : %u", gs_RenderEngine->g_DebugDrawVertexBufferGPU.size());
                                ImGui::Text("g_DebugDrawIndexBufferGPU.size()  : %u", gs_RenderEngine->g_DebugDrawIndexBufferGPU.size());
                                ImGui::Text("g_Textures.size()                 : %u", gs_RenderEngine->g_Textures.size());
                                ImGui::Text("g_GlobalMeshBuffers.VtxBuffer.size() : %u", gs_RenderEngine->g_GlobalMeshBuffers.VtxBuffer.size());
                                ImGui::Text("g_GlobalMeshBuffers.IdxBuffer.size() : %u", gs_RenderEngine->g_GlobalMeshBuffers.IdxBuffer.size());
                                ImGui::Separator();
								ImGui::TextColored({ 0.0,1.0,0.0,1.0 }, "Shader Debug Tool");
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
								ImGui::Separator();
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
                                ImGui::PopID();

                                // TODO?
                                ImGui::EndTabItem();
                            }

                            ImGui::EndTabBar();
                        }//ImGui::BeginTabBar
                    }//ImGui::Begin

                    ImGui::End();
                }

                {
                    PROFILE_SCOPED("ImGui::Render");
                    ImGui::Render();  // Rendering UI
                }
                gs_RenderEngine->DrawGUI();

                gs_RenderEngine->Present();
            }

            //finish for all windows
            ImGui::EndFrame();
            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

        }
    }

    //----------------------------------------------------------------------------------------------------
    // Application Shutdown
    //----------------------------------------------------------------------------------------------------

    gs_RenderEngine->DestroyImGUI();
    ImGui::DestroyContext(ImGui::GetCurrentContext());

    if (gs_RenderEngine)
        delete gs_RenderEngine;
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
    std::unique_ptr<ModelData> plane{ gs_RenderEngine->LoadMeshFromBuffers(defaultPlaneMesh.m_VertexBuffer, defaultPlaneMesh.m_IndexBuffer, nullptr) };
    std::unique_ptr<ModelData> box{ gs_RenderEngine->LoadMeshFromBuffers(defaultCubeMesh.m_VertexBuffer, defaultCubeMesh.m_IndexBuffer, nullptr) };


}
