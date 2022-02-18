
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
        response  = std::tolower(response);
        if (response != 'y' && response != 'n'){
            std::cout<< "Invalid input["<< response<< "]please try again"<< std::endl;
            response = 0;           
        }
    }
    return response == 'n' ? false : true;
}


int main(int argc, char argv[])
{

    Window mainWindow;
    mainWindow.Init();

    
    //handling winOS messages
    // This will handle inputs and pass it to our input callback
    MSG msg; // this is a good flavouring for fried rice
    while( 1 )  // infinite loop
    {
        // 1. Check if theres a message in the WinOS message queue and remove it using PM_REMOVE
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ) // process every single message in the queue
        {
             //process the message
            if( msg.message == WM_QUIT )
            {
                break;  // BREAK OUT OF INFINITE LOOP
                        // if user is trying to quit!
            }
            else
            {
                // Parses and translates the message for WndProc function
                TranslateMessage(&msg);
                // now we dispatch the compatible message to our WndProc function.
                DispatchMessage(&msg);
            }
        }
        else
        {
            //run our core game loop
            //updatePhys();
            //draw();
        }
    }
    

    oGFX::SetupInfo setupSpec;

    //setupSpec.debug = BoolQueryUser("Do you want debugging?");
    //setupSpec.renderDoc = BoolQueryUser("Do you want renderdoc?");
    setupSpec.debug = true;
    setupSpec.renderDoc = false;

	VulkanRenderer renderer;
    try
    {
        renderer.CreateInstance(setupSpec);
        renderer.CreateSurface(mainWindow);
        renderer.AcquirePhysicalDevice();
        renderer.CreateLogicalDevice();
        std::cout << "Created vulkan instance!"<< std::endl;
    }
    catch (...)
    {
        std::cout << "Cannot create vulkan instance!"<< std::endl;
    }

    std::cout << "Exiting application..."<< std::endl;

}
