
#include "gpuCommon.h"
#include "VulkanRenderer.h"

#include <iostream>
#include <cctype>

int main(int argc, char argv[])
{
    oGFX::SetupInfo setupSpec;

    char response {0} ;
    std::cout<< "Do you want debugging? [y/n]"<< std::endl;
    while (! response ){
        std::cin>> response;
        response  = std::tolower(response);
        if (response != 'y' && response != 'n'){
            std::cout<< "Invalid input["<< response<< "]please try again"<< std::endl;
            response = 0;
        }
    }
    setupSpec.debug = response == 'n' ? false : true;

    response = 0;
    std::cout<< "Do you want renderdoc? [y/n]"<< std::endl;
    while (! response ){
        std::cin>> response;
        response  = std::tolower(response);
        if (response != 'y' && response != 'n'){
            std::cout<< "Invalid input please try again"<< std::endl;
            response = 0;
        }
    }
    setupSpec.renderDoc = response == 'n' ? false : true;


	VulkanRenderer renderer;
    try
    {
	    renderer.CreateInstance(setupSpec);
        std::cout << "Created vulkan instance!"<< std::endl;
    }
    catch (...)
    {
        std::cout << "Cannot create vulkan instance!"<< std::endl;
    }

    std::cout << "Exiting application..."<< std::endl;

}
