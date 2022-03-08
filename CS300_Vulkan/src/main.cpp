
#define NOMINMAX
#if defined(_WIN32)
#include <windows.h>
#endif

#include "gpuCommon.h"
#include "VulkanRenderer.h"

#include <iostream>
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
    uint32_t obj = renderer.CreateMeshModel(verts, indices);

    //handling winOS messages
    // This will handle inputs and pass it to our input callback
    while( mainWindow.windowShouldClose == false )  // infinite loop
    {
        while(Window::PollEvents());

        renderer.Draw();
    }
    

   

    std::cout << "Exiting application..."<< std::endl;

}
