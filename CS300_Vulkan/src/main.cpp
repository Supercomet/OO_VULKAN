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
#include "input.h"


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
        renderer.Init(setupSpec, mainWindow);

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
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, { 0.3f,0.3f,0.3f } ,{ 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, { 0.0f,0.0f,1.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, { 0.0f,1.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, { 0.0f,1.0f,1.0f }, { 0.0f,0.0f } },
                                          
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, { 1.0f,0.0f,0.0f } ,{ 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, { 1.0f,0.0f,1.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, { 1.0f,1.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, { 1.0f,1.0f,1.0f }, { 0.0f,0.0f } }
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

    uint32_t Object = renderer.CreateMeshModel("Models/TextObj.obj");

   uint32_t obj = renderer.CreateMeshModel(verts, indices);
   glm::mat4 xform{ 1.0f };
   xform = glm::translate(xform, glm::vec3(-3.0f, 0.0f, -3.0f));
   xform = glm::scale(xform, glm::vec3{ 4.0f,4.0f,4.0f });
   renderer.UpdateModel(obj,xform );

   uint32_t obj2 = renderer.CreateMeshModel(boxverts, boxindices);
   renderer.UpdateModel(obj2, xform);

    float angle = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();

    renderer.camera.SetRotation(glm::vec3(-4.35f, 16.25f, 0.0f));
    renderer.camera.SetRotationSpeed(0.5f);
    renderer.camera.SetPosition(glm::vec3(0.1f, -2.0f, -10.5f));

    glm::vec3 pos{0.1f, 1.1f, -3.5f};
    //handling winOS messages
    // This will handle inputs and pass it to our input callback
    while( mainWindow.windowShouldClose == false )  // infinite loop
    {
        Input::Begin();
        while(Window::PollEvents());

        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>( now - lastTime).count();
        lastTime = now;

        renderer.camera.Update(deltaTime);
        float speed = 0.05f;


        renderer.camera.SetPerspective(60.0f, (float)mainWindow.m_width / (float)mainWindow.m_height, 0.1f, 256.0f);
        auto mousedel = Input::GetMouseDelta();

        //pos.y += mousedel.y* 0.001f;
        //pos.x += mousedel.x * 0.001f;
        //renderer.camera.SetPosition(pos);
        
        float wheelDelta = Input::GetMouseWheel();

        renderer.camera.Translate(glm::vec3(0.0f, 0.0f, wheelDelta * 0.005f));

        if (Input::GetMouseHeld(MOUSE_LEFT)) {
            renderer.camera.Rotate(glm::vec3(-mousedel.y * renderer.camera.rotationSpeed, -mousedel.x * renderer.camera.rotationSpeed, 0.0f));
        }

        if (Input::GetKeyHeld(KEY_W))
        {
            renderer.camera.Translate(glm::vec3{ 0.0f,0.0f,1.0f }* speed);
            renderer.camera.keys.up = true;
        }
        if (Input::GetKeyHeld(KEY_S))
        {
            renderer.camera.Translate(glm::vec3{ 0.0f,0.0f,-1.0f }* speed);
            renderer.camera.keys.down = true;
        }
        if (Input::GetKeyHeld(KEY_A))
        {
            renderer.camera.Translate(glm::vec3{ 1.0f,0.0f,0.0f }* speed);
            renderer.camera.keys.left = true;
        }
        if (Input::GetKeyHeld(KEY_D))
        {
            renderer.camera.Translate(glm::vec3{ -1.0f,0.0f,0.0f }* speed);
            renderer.camera.keys.right = true;
        }
        if (Input::GetKeyHeld(KEY_Q))
        {
            renderer.camera.Translate(glm::vec3{ 0.0f,1.0f,0.0f }* speed);
            renderer.camera.keys.right = true;
        }

        if (Input::GetKeyHeld(KEY_E))
        {
            renderer.camera.Translate(glm::vec3{ 0.0f,-1.0f,0.0f } * speed);
            renderer.camera.keys.right = true;
        }

        angle += 5.0f * deltaTime;
        if (angle > 360.0f) { angle -= 360.0f; }

        glm::mat4 testMat = glm::mat4(1.0f);
        //testMat = glm::translate(testMat, {0.0f,0.0f,0.0f});
        //testMat = glm::rotate(testMat, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
        testMat = glm::scale(testMat, glm::vec3{ 0.5f,0.5f,0.5f });
        renderer.UpdateModel(Object, testMat);


        xform = { 1.0f };
        xform = glm::translate(xform, glm::vec3(3.0f, 0.0f, 3.0f));
        xform = glm::rotate(xform,angle, glm::vec3(0.0f, 1.0f, 0.0f));
        xform = glm::scale(xform, glm::vec3{ 3.0f,3.0f,3.0f });
        renderer.UpdateModel(obj2, xform);

        renderer.Draw();
    }   

    std::cout << "Exiting application..."<< std::endl;

}
