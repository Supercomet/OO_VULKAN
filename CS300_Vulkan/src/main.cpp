#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include "gpuCommon.h"
#include "VulkanRenderer.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <iostream>
#include <chrono>
#include <cctype>
#include <thread>
#include <random>

#include "window.h"
#include "input.h"

#include "Tests_Assignment1.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <imgui.h>
#include <backends/imgui_impl_vulkan.h>
#include <backends/imgui_impl_win32.h>

bool BoolQueryUser(const char * str)
{
    char response {0} ;
    std::cout<< str << " [y/n]"<< std::endl;
    while (! response ){
        std::cin>> response;
        response  = static_cast<char>(std::tolower(response));
        if (response != 'y' && response != 'n'){
            std::cout<< "Invalid input["<< response<< "]please try again"<< std::endl;
            response = 0;           
        }
    }
    return response == 'n' ? false : true;
}


int main(int argc, char argv[])
{
    (void)argc;
    (void)argv;

    _CrtDumpMemoryLeaks();
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    //_CrtSetBreakAlloc(2383);

    //RunAllTests();

    Window mainWindow(1440,900);
    mainWindow.Init();

    
    oGFX::SetupInfo setupSpec;

    //setupSpec.debug = BoolQueryUser("Do you want debugging?");
    //setupSpec.renderDoc = BoolQueryUser("Do you want renderdoc?");
    setupSpec.debug = true;
    setupSpec.renderDoc = true;

    VulkanRenderer renderer;
    try
    {
        renderer.Init(setupSpec, mainWindow);

        std::cout << "Created vulkan instance!"<< std::endl;
    }
    catch (...)
    {
        std::cout << "Cannot create vulkan instance!"<< std::endl;
    }

    uint32_t colour = 0xFFFFFFFF; // ABGR
    renderer.CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&colour));

    std::vector<oGFX::Vertex>quadVerts{
            oGFX::Vertex{ {-0.5,-0.5,0.0}, { 1.0f,0.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
            oGFX::Vertex{ { 0.5,-0.5,0.0}, { 0.0f,1.0f,0.0f }, { 0.0f,1.0f,0.0f }, { 0.0f,0.0f } },
            oGFX::Vertex{ { 0.0, 0.5,0.0}, { 0.0f,0.0f,1.0f }, { 0.0f,0.0f,1.0f }, { 0.0f,0.0f } }
    };
    std::vector<uint32_t>quadIndices{
        0,1,2
    };

    std::vector<oGFX::Vertex>boxverts{
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, { 0.3f,0.3f,0.3f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, { 0.0f,0.0f,1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, { 0.0f,1.0f,0.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, { 0.0f,1.0f,1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
                                                           
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, { 1.0f,0.0f,0.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, { 1.0f,0.0f,1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, { 1.0f,1.0f,0.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, { 1.0f,1.0f,1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } }
    };
    std::vector<uint32_t> boxindices{
         1,0,2
        ,3,1,2
        ,5,1,3
        ,5,3,7
        ,4,5,7
        ,4,7,6
        ,0,4,6
        ,0,6,2
        ,3,2,6
        ,3,6,7
        ,5,4,0
        ,5,0,1
    };


    //uint32_t yes = renderer.LoadMeshFromFile("Models/TextObj.obj");
    //uint32_t yes = renderer.LoadMeshFromFile("Models/Skull_textured.fbx");

    uint32_t ball = renderer.LoadMeshFromFile("Models/icosphere.obj");
    uint32_t box = renderer.LoadMeshFromBuffers(boxverts, boxindices, nullptr);
    uint32_t triangle = renderer.LoadMeshFromBuffers(quadVerts, quadIndices, nullptr);

    glm::mat4 id(1.0f);

    VulkanRenderer::EntityDetails ed;
    ed.modelID = ball;
    ed.pos = { 2.0f,0.0f,2.0f };
    renderer.entities.push_back(ed);
    ed.modelID = box;
    ed.pos = { -2.0f,0.0f,-2.0f };
    renderer.entities.push_back(ed);
    ed.modelID = triangle;
    ed.pos = { 0.0f,0.0f,0.0f };
    renderer.entities.push_back(ed);

    
    //uint32_t Object = renderer.CreateMeshModel("Models/TextObj.obj");
    //uint32_t obj = renderer.CreateMeshModel(verts, indices);
   //renderer.CreateTexture("Textures/TD_Checker_Base_Color.png");
   //renderer.CreateTexture("TD_Checker_Mixed_AO.png");
   //renderer.CreateTexture("TD_Checker_Normal_OpenGL.png");
   //renderer.CreateTexture("TD_Checker_Roughness.png");

   //auto alb = renderer.CreateTexture("Textures/TD_Checker_Base_Color.dds");
   //auto norm =renderer.CreateTexture("Textures/TD_Checker_Normal_OpenGL.dds");
   //auto occlu =renderer.CreateTexture("Textures/TD_Checker_Mixed_AO.dds");
   //auto rough =renderer.CreateTexture("Textures/TD_Checker_Roughness.dds");
   //renderer.SetMeshTextures(yes, alb, norm, occlu, rough);

   //renderer.SubmitMesh(yes, position);

   //create a hundred random textures because why not
   //std::default_random_engine rndEngine(123456);
   //std::uniform_int_distribution<uint32_t> uniformDist( 0xFF000000, 0xFFFFFFFF );
   //std::vector<oGFX::InstanceData> instanceData;
   //constexpr size_t numTex = 100;
   //constexpr size_t dims = 2;
   //std::vector<uint32_t> bitmap(dims*dims);
   //for (size_t i = 0; i < numTex; i++)
   //{
   //    for (size_t x = 0; x < bitmap.size(); x++)
   //    {
   //    uint32_t colour = uniformDist(rndEngine); // ABGR
   //     bitmap[x] = colour;
   //    }
   //    renderer.CreateTexture(dims, dims, reinterpret_cast<unsigned char*>(bitmap.data()));
   //}

   glm::mat4 xform{ 1.0f };
   xform = glm::translate(xform, glm::vec3(-3.0f, 0.0f, -3.0f));
   xform = glm::scale(xform, glm::vec3{ 4.0f,4.0f,4.0f });

    float angle = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();

    renderer.camera.type = Camera::CameraType::lookat;
    renderer.camera.target = glm::vec3(0.01f, 0.0f, 0.0f);
    renderer.camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.camera.SetRotationSpeed(0.5f);
    renderer.camera.SetPosition(glm::vec3(0.1f, -2.0f, -10.5f));

    bool freezeLight = false;

    glm::vec3 pos{0.1f, 1.1f, -3.5f};
    //handling winOS messages
    // This will handle inputs and pass it to our input callback
    while( mainWindow.windowShouldClose == false )  // infinite loop
    {
        //reset keys
        Input::Begin();
        while(Window::PollEvents());

        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>( now - lastTime).count();
        lastTime = now;

        renderer.camera.Update(deltaTime);
        float speed = 0.05f;

        if (mainWindow.m_width != 0 && mainWindow.m_height != 0)
        {
            renderer.camera.SetPerspective(60.0f, (float)mainWindow.m_width / (float)mainWindow.m_height, 0.1f, 10000.0f);
        }

        auto mousedel = Input::GetMouseDelta();
        float wheelDelta = Input::GetMouseWheel();

        renderer.camera.ChangeDistance(wheelDelta * -0.001f);
        auto frontVec = renderer.camera.GetFront();
        auto rightVec = renderer.camera.GetRight();
        auto upVec = renderer.camera.GetUp();

        if (Input::GetMouseHeld(MOUSE_RIGHT)) {
            renderer.camera.Rotate(glm::vec3(mousedel.y * renderer.camera.rotationSpeed, mousedel.x * renderer.camera.rotationSpeed, 0.0f));
            renderer.camera.keys.up = true;
        }

        if (Input::GetKeyTriggered(KEY_SPACE))
        {
            freezeLight = !freezeLight;
        }
        if (freezeLight == false)
        {
            renderer.light.position = renderer.camera.position;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (renderer.PrepareFrame() == true)
        {
            renderer.Draw();
            renderer.PrePass();
            renderer.SimplePass();
            //renderer.RecordCommands(renderer.swapchainImageIndex);

            // Create a dockspace over the mainviewport so that we can dock stuff
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), 
                ImGuiDockNodeFlags_PassthruCentralNode // make the dockspace transparent
                | ImGuiDockNodeFlags_NoDockingInCentralNode // dont allow docking in the central area
            );
            
            //ImGui::ShowDemoWindow();
            
            ImGui::Begin("Test");
            for (auto& entity: renderer.entities)
            {
                switch (entity.modelID)
                {
                case 0:
                ImGui::Text("Sphere");
                break;
                case 1:
                ImGui::Text("Box");
                break;
                case 2:
                ImGui::Text("Triangle");
                break;
                default:
                break;
                }
                auto id = std::to_string(entity.modelID);
                ImGui::DragFloat3(std::string("Position"+ id).c_str(), glm::value_ptr(entity.pos));
                ImGui::DragFloat3(("Scale"+id).c_str(),  glm::value_ptr(entity.scale));
                ImGui::DragFloat(("theta"+id).c_str(),  &entity.rot);
                ImGui::DragFloat3(("Rotation Vec"+id).c_str(),  glm::value_ptr(entity.rotVec));
            }
            ImGui::End();
            renderer.DrawGUI();

            renderer.Present();
        }

        //finish for all windows
        ImGui::EndFrame();
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {            
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

    }   

    std::cout << "Exiting application..."<< std::endl;

}
