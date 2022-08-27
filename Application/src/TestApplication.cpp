#include "TestApplication.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include "gpuCommon.h"
#include "Vulkanrenderer.h"

#include "window.h"
#include "input.h"

#include "Tests_Assignment1.h"

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

static float* gizmoHijack = nullptr; // TODO: Clean this up...

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
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

        ImGui::StyleColorsDark(); // Setup Dear ImGui style
        gs_RenderEngine->InitImGUI();

        std::cout << "Created Vulkan instance!" << std::endl;
    }
    catch (...)
    {
        std::cout << "Fucked!" << std::endl;
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

    BindlessTextureIndex diffuseTexture0 = gs_RenderEngine->CreateTexture("Textures/7/d.png");
    BindlessTextureIndex diffuseTexture1 = gs_RenderEngine->CreateTexture("Textures/8/d.png");
    BindlessTextureIndex diffuseTexture2 = gs_RenderEngine->CreateTexture("Textures/13/d.png");
    BindlessTextureIndex diffuseTexture3 = gs_RenderEngine->CreateTexture("Textures/23/d.png");

    BindlessTextureIndex roughnessTexture0 = gs_RenderEngine->CreateTexture("Textures/7/r.png");
    BindlessTextureIndex roughnessTexture1 = gs_RenderEngine->CreateTexture("Textures/8/r.png");
    BindlessTextureIndex roughnessTexture2 = gs_RenderEngine->CreateTexture("Textures/13/r.png");
    BindlessTextureIndex roughnessTexture3 = gs_RenderEngine->CreateTexture("Textures/23/r.png");

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Models
    //----------------------------------------------------------------------------------------------------

    auto defaultPlaneMesh = CreateDefaultPlaneXZMesh();
    auto defaultCubeMesh = CreateDefaultCubeMesh();
    std::unique_ptr<Model> model_plane{ gs_RenderEngine->LoadMeshFromBuffers(defaultPlaneMesh.m_VertexBuffer, defaultPlaneMesh.m_IndexBuffer, nullptr) };
    std::unique_ptr<Model> model_box{ gs_RenderEngine->LoadMeshFromBuffers(defaultCubeMesh.m_VertexBuffer, defaultCubeMesh.m_IndexBuffer, nullptr) };

    std::unique_ptr<Model> character_diona{ gs_RenderEngine->LoadMeshFromFile("Models/diona.fbx") };
    std::unique_ptr<Model> character_qiqi{ gs_RenderEngine->LoadMeshFromFile("Models/qiqi.fbx") };

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Scene Objects
    //----------------------------------------------------------------------------------------------------

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
        VulkanRenderer::EntityDetails ed;
        ed.name = "Plane";
        ed.entityID = FastRandomMagic();
        ed.modelID = model_plane->gfxIndex;
        ed.position = { 0.0f,0.0f,0.0f };
        ed.scale = { 15.0f,1.0f,15.0f };
        gs_RenderEngine->entities.push_back(ed);
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

            VulkanRenderer::EntityDetails ed;
            ed.name = "Plane_" + std::to_string(i);
            ed.entityID = FastRandomMagic();
            ed.modelID = model_plane->gfxIndex;
            ed.position = positions[i];
            ed.scale = { 15.0f,1.0f,15.0f };
            ed.bindlessGlobalTextureIndex_Albedo = diffuseBindlessTextureIndexes[i / 2];
            ed.bindlessGlobalTextureIndex_Roughness = roughnessBindlessTextureIndexes[i / 2];
            gs_RenderEngine->entities.push_back(ed);
        }
    }

    if (character_diona)
    {
        VulkanRenderer::EntityDetails ed;
        ed.modelID = character_diona->gfxIndex;
        ed.name = "diona";
        ed.entityID = FastRandomMagic();
        ed.position = { 0.0f,0.0f,0.0f };
        ed.scale = { 1.0f,1.0f,1.0f };
        ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        gs_RenderEngine->entities.push_back(ed);
    }

    if (character_qiqi)
    {
        VulkanRenderer::EntityDetails ed;
        ed.modelID = character_qiqi->gfxIndex;
        ed.name = "qiqi";
        ed.entityID = FastRandomMagic();
        ed.position = { 1.0f,0.0f,0.0f };
        ed.scale = { 1.0f,1.0f,1.0f };
        ed.bindlessGlobalTextureIndex_Albedo = diffuseTexture3;
        gs_RenderEngine->entities.push_back(ed);
    }

    // Transfer to Graphics World
    for (auto& e : gs_RenderEngine->entities)
    {
        AABB ab;
        auto& model = gs_RenderEngine->models[e.modelID];

        UpdateBV(gs_RenderEngine->models[e.modelID].cpuModel, e);

        glm::mat4 xform{ 1.0f };
        xform = glm::translate(xform, e.position);
        xform = glm::rotate(xform, glm::radians(e.rot), e.rotVec);
        xform = glm::scale(xform, e.scale);
        auto id = gs_GraphicsWorld.CreateObjectInstance(
            ObjectInstance{
                e.name,
                e.position,
                e.scale,
                e.rot,
                e.rotVec,
                e.bindlessGlobalTextureIndex_Albedo,
                e.bindlessGlobalTextureIndex_Normal,
                e.bindlessGlobalTextureIndex_Roughness,
                e.bindlessGlobalTextureIndex_Metallic,
                xform,
                e.modelID,
                e.entityID
            }
        );
    }

    //----------------------------------------------------------------------------------------------------
    // Setup Initial Camera
    //----------------------------------------------------------------------------------------------------

    gs_RenderEngine->camera.m_TargetPosition = glm::vec3(0.01f, 0.0f, 0.0f);
    gs_RenderEngine->camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    gs_RenderEngine->camera.SetRotationSpeed(0.5f);
    gs_RenderEngine->camera.SetPosition(glm::vec3(0.1f, 10.0f, 10.5f));
    gs_RenderEngine->camera.movementSpeed = 5.0f;
    gs_RenderEngine->camera.SetPerspective(60.0f, (float)mainWindow.m_width / (float)mainWindow.m_height, 0.1f, 10000.0f);
    gs_RenderEngine->camera.Rotate(glm::vec3(1 * gs_RenderEngine->camera.rotationSpeed, 1 * gs_RenderEngine->camera.rotationSpeed, 0.0f));
    gs_RenderEngine->camera.m_CameraMovementType = Camera::CameraMovementType::firstperson;

    auto lastTime = std::chrono::high_resolution_clock::now();
    static bool freezeLight = false;

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

            //reset keys
            Input::Begin();
            while (Window::PollEvents());

            //------------------------------
            // Camera Controller
            //------------------------------
            {
                auto& camera = gs_RenderEngine->camera;

                camera.keys.left = Input::GetKeyHeld(KEY_A) ? true : false;
                camera.keys.right = Input::GetKeyHeld(KEY_D) ? true : false;
                camera.keys.down = Input::GetKeyHeld(KEY_S) ? true : false;
                camera.keys.up = Input::GetKeyHeld(KEY_W) ? true : false;
                camera.Update(deltaTime);

                camera.SetAspectRatio((float)mainWindow.m_width / (float)mainWindow.m_height);

                /*
                if (mainWindow.m_width != 0 && mainWindow.m_height != 0)
                {
                    camera.SetPerspective(60.0f, (float)mainWindow.m_width / (float)mainWindow.m_height, 0.1f, 10000.0f);
                }
                */

                auto mousedel = Input::GetMouseDelta();
                float wheelDelta = Input::GetMouseWheel();
                if (Input::GetMouseHeld(MOUSE_RIGHT))
                {
                    camera.Rotate(glm::vec3(-mousedel.y * camera.rotationSpeed, mousedel.x * camera.rotationSpeed, 0.0f));
                }
                if (gs_RenderEngine->camera.m_CameraMovementType == Camera::CameraMovementType::lookat)
                {
                    camera.ChangeDistance(wheelDelta * -0.001f);
                }

                camera.UpdateProjectionMatrix();
            }

            if (Input::GetKeyTriggered(KEY_SPACE))
            {
                freezeLight = !freezeLight;
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            if (gs_RenderEngine->PrepareFrame() == true)
            {
                PROFILE_SCOPED("gs_RenderEngine->PrepareFrame() == true");

                gs_RenderEngine->timer += deltaTime;
                if (freezeLight == false)
                {
                    // TODO: turn into proper entities
                    gs_RenderEngine->UpdateLights(deltaTime);
                }

                // Upload CPU light data to GPU. Ideally this should only contain lights that intersects the camera frustum.
                gs_RenderEngine->UploadLights();

                // Render the frame
                gs_RenderEngine->RenderFrame();

                // Create a dockspace over the mainviewport so that we can dock stuff
                ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(),
                    ImGuiDockNodeFlags_PassthruCentralNode // make the dockspace transparent
                    | ImGuiDockNodeFlags_NoDockingInCentralNode // dont allow docking in the central area
                );

                //ImGui::ShowDemoWindow();

                // ImGuizmo
                if (gizmoHijack)
                {
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
                    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
                    const auto& viewMatrix = gs_RenderEngine->camera.matrices.view;
                    const auto& projMatrix = gs_RenderEngine->camera.matrices.perspective;

                    static glm::mat4x4 localToWorld{ 1.0f };
                    float* matrixPtr = glm::value_ptr(localToWorld);

                    //if (gizmoHijack) // Fix me!
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
                            gizmoHijack[0] = position[0];
                            gizmoHijack[1] = position[1];
                            gizmoHijack[2] = position[2];
                        }
                    }
                }
                bool renderGraphicsWorld = gs_RenderEngine->currWorld;
                // Display ImGui Window
                {
                    PROFILE_SCOPED("ImGuiSceneHelper");
                    if (ImGui::Begin("Scene Helper"))
                    {
                        if (ImGui::Checkbox("RenderGraphicsWorld", &renderGraphicsWorld))
                        {
                            if (renderGraphicsWorld)
                            {
                                gs_RenderEngine->SetWorld(&gs_GraphicsWorld);
                            }
                            else
                            {
                                gs_RenderEngine->SetWorld(nullptr);
                            }
                        }
                        if (ImGui::BeginTabBar("SceneHelperTabBar"))
                        {
                            if (ImGui::BeginTabItem("Entity"))
                            {
                                if (ImGui::SmallButton("Create Cube"))
                                {
                                    VulkanRenderer::EntityDetails entity;
                                    entity.position = { 2.0f,2.0f,2.0f };
                                    entity.scale = { 1.0f,1.0f,1.0f };
                                    entity.modelID = model_box->gfxIndex;
                                    entity.entityID = FastRandomMagic();
                                    gs_RenderEngine->entities.emplace_back(entity);
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

                                        VulkanRenderer::EntityDetails entity;
                                        entity.position = { pos.x, glm::abs(pos.y), pos.z };
                                        entity.scale = { 1.0f,1.0f,1.0f };
                                        entity.modelID = model_box->gfxIndex;
                                        entity.entityID = FastRandomMagic();
                                        gs_RenderEngine->entities.emplace_back(entity);
                                    }
                                }

                                ImGui::Text("Total Entities: %u", gs_RenderEngine->entities.size());

                                if (ImGui::TreeNode("Entity List"))
                                {
                                    for (auto& entity : gs_RenderEngine->entities)
                                    {
                                        ImGui::PushID(entity.entityID);

                                        ImGui::BulletText("[ID:%u] ", entity.entityID);
                                        ImGui::SameLine();
                                        ImGui::Text(entity.name.c_str());
                                        ImGui::DragFloat3("Position", glm::value_ptr(entity.position), 0.01f);
                                        {
                                            if (ImGui::BeginPopupContextItem("Gizmo hijacker"))
                                            {
                                                if (ImGui::Selectable("Set ptr Gizmo"))
                                                {
                                                    // Shamelessly point to this property (very unsafe, but quick to test shit and speed up iteration time)
                                                    gizmoHijack = glm::value_ptr(entity.position);
                                                }
                                                ImGui::EndPopup();
                                            }
                                        }

                                        ImGui::DragFloat3("Scale", glm::value_ptr(entity.scale), 0.01f);
                                        ImGui::DragFloat3("Rotation Axis", glm::value_ptr(entity.rotVec));
                                        ImGui::DragFloat("Theta", &entity.rot);
                                        // TODO: We should be using quaternions.........

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
                                ImGui::DragFloat3("ViewPos", glm::value_ptr(gs_RenderEngine->lightUBO.viewPos));
                                ImGui::Separator();
                                for (int i = 0; i < 6; ++i)
                                {
                                    ImGui::PushID(i);
                                    auto& light = gs_RenderEngine->m_HardcodedOmniLights[i];
                                    ImGui::DragFloat3("Position", glm::value_ptr(light.position), 0.01f);
                                    {
                                        if (ImGui::BeginPopupContextItem("Gizmo hijacker"))
                                        {
                                            if (ImGui::Selectable("Set ptr Gizmo"))
                                            {
                                                // Shamelessly point to this property (very unsafe, but quick to test shit and speed up iteration time)
                                                gizmoHijack = glm::value_ptr(light.position);
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
                                            auto& light = gs_RenderEngine->m_HardcodedOmniLights[i];
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
                                ImGui::DragFloat3("Position", glm::value_ptr(camera.position));
                                ImGui::DragFloat3("Rotation", glm::value_ptr(camera.rotation));
                                ImGui::DragFloat3("Target", glm::value_ptr(camera.m_TargetPosition));
                                ImGui::DragFloat("Distance", &camera.m_TargetDistance);

                                bool fps = camera.m_CameraMovementType == Camera::CameraMovementType::firstperson;
                                if (ImGui::Checkbox("FPS", &fps))
                                {
                                    camera.m_CameraMovementType = fps ? Camera::CameraMovementType::firstperson : Camera::CameraMovementType::lookat;
                                }

                                ImGui::DragFloat("RotationSpeed", &camera.rotationSpeed);
                                ImGui::DragFloat("MovementSpeed", &camera.movementSpeed);

                                ImGui::EndTabItem();
                            }//ImGui::BeginTabItem

                            if (ImGui::BeginTabItem("Settings"))
                            {
                                // TODO?
                                ImGui::EndTabItem();
                            }

                            ImGui::EndTabBar();
                        }//ImGui::BeginTabBar
                    }//ImGui::Begin

                    ImGui::End();
                }

                //
                ImGui::Render();  // Rendering UI
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
    std::unique_ptr<Model> plane{ gs_RenderEngine->LoadMeshFromBuffers(defaultPlaneMesh.m_VertexBuffer, defaultPlaneMesh.m_IndexBuffer, nullptr) };
    std::unique_ptr<Model> box{ gs_RenderEngine->LoadMeshFromBuffers(defaultCubeMesh.m_VertexBuffer, defaultCubeMesh.m_IndexBuffer, nullptr) };


}
