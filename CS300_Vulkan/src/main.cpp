#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define NOMINMAX
#if defined(_WIN32)
#include <windows.h>
#endif

#include "gpuCommon.h"
#include "VulkanRenderer.h"
#include "glm/gtc/matrix_transform.hpp"

#include <iostream>
#include <chrono>
#include <cctype>
#include <thread>

#include "window.h"


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

    Window mainWindow;
    mainWindow.Init();

    
    oGFX::SetupInfo setupSpec;

    //setupSpec.debug = BoolQueryUser("Do you want debugging?");
    //setupSpec.renderDoc = BoolQueryUser("Do you want renderdoc?");
    setupSpec.debug = true;
    setupSpec.renderDoc = true;

    VulkanRenderer renderer;
    try
    {
        renderer.CreateInstance(setupSpec);
        renderer.CreateSurface(mainWindow);
        renderer.AcquirePhysicalDevice();
        renderer.CreateLogicalDevice();
        renderer.SetupSwapchain();
        renderer.CreateRenderpass();
        renderer.CreateDescriptorSetLayout();
        renderer.CreatePushConstantRange();
        renderer.CreateGraphicsPipeline();
        renderer.CreateDepthBufferImage();
        renderer.CreateFramebuffers();
        renderer.CreateCommandPool();
        renderer.CreateCommandBuffers();
        renderer.CreateTextureSampler();
        renderer.CreateUniformBuffers();

        renderer.CreateDescriptorPool();
        renderer.CreateDescriptorSets();
        renderer.CreateSynchronisation();

        std::cout << "Created vulkan instance!"<< std::endl;
    }
    catch (...)
    {
        std::cout << "Cannot create vulkan instance!"<< std::endl;
    }

    uint32_t colour = 0xffffffff;
    renderer.CreateTexture(1, 1, reinterpret_cast<const unsigned char*>(&colour));

    std::vector<oGFX::Vertex>verts{
            oGFX::Vertex{ {-0.5,-0.5,0.0}, { 1.0f,0.0f,0.0f } ,{ 0.0f,0.0f } },
            oGFX::Vertex{ { 0.5,-0.5,0.0} ,{ 0.0f,1.0f,0.0f }, { 0.0f,0.0f } },
            oGFX::Vertex{ { 0.0, 0.5,0.0}, { 0.0f,0.0f,1.0f }, { 0.0f,0.0f } }
    };
    std::vector<uint32_t> indices{
        0,1,2
    };

    std::vector<oGFX::Vertex>boxverts{
        oGFX::Vertex{ {-0.5,-0.5,0.0}, { 0.5f,0.0f,0.0f } ,{ 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5,-0.5,0.0} ,{ 0.0f,0.5f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ {-0.5, 0.5,0.0}, { 0.0f,0.0f,0.5f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5, 0.5,0.0}, { 0.0f,0.5f,0.5f }, { 0.0f,0.0f } }
    };
    std::vector<uint32_t> boxindices{
        0,1,2
       ,1,3,2
    };

    uint32_t Object = renderer.CreateMeshModel("Models/TextObj.obj");
    auto objmat = glm::translate(glm::mat4(1.0f), glm::vec3(-0.2f, 0.0f, -0.5f));
    objmat = glm::scale(objmat, glm::vec3(0.1f, 0.1f, 0.1f));
    renderer.UpdateModel(Object, objmat);

    uint32_t obj = renderer.CreateMeshModel(verts, indices);
    renderer.UpdateModel(obj, glm::translate(glm::mat4(1.0f), glm::vec3(-0.f, -0.5f, -0.5f)));

    uint32_t obj2 = renderer.CreateMeshModel(boxverts, boxindices);
    renderer.UpdateModel(obj2, glm::translate(glm::mat4(1.0f), glm::vec3(-0.2f, 0.0f, -0.5f)));
    //renderer.updateModel(Object, testMat);

    float angle = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();

    //handling winOS messages
    // This will handle inputs and pass it to our input callback
    while( mainWindow.windowShouldClose == false )  // infinite loop
    {
        while(Window::PollEvents());

        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>( now - lastTime).count();
        lastTime = now;

        angle += 180.f * deltaTime;
        if (angle > 360.0f) { angle -= 360.0f; }

        glm::mat4 testMat = glm::mat4(1.0f);
        testMat = glm::translate(testMat,  glm::vec3(0.5f, 0.0f, -0.5));
        testMat = glm::rotate(testMat, glm::radians(angle), glm::vec3(1.0f, 1.0f, 0.0f));
        testMat = glm::scale(testMat, glm::vec3{ 0.1f,0.1f,0.1f });
        renderer.UpdateModel(Object, testMat);

        renderer.Draw();
    }
    

   

    std::cout << "Exiting application..."<< std::endl;

}
