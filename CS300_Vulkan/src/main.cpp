#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#endif

#include "gpuCommon.h"
#include "VulkanRenderer.h"


#include <iostream>
#include <iomanip>
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

#include "IcoSphereCreator.h"
#include "Tree.h"

#include <numeric>
//#include <algorithm>

#include "BoudingVolume.h"
std::ostream& operator<<(std::ostream& os, const glm::vec3& vec)
{
    os << std::setprecision(4) << "[" << vec.x << "," << vec.y << "," << vec.z << "]";
    return os;
}

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
        return 0;
    }

    uint32_t colour = 0xFFFFFFFF; // ABGR
    renderer.CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&colour));
    renderer.CreateTexture(1, 1, reinterpret_cast<unsigned char*>(&colour));

    std::vector<oGFX::Vertex>planeVerts{
        oGFX::Vertex{ {-0.5f, 0.0f ,-0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5f, 0.0f ,-0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 1.0f,0.0f } },
        oGFX::Vertex{ { 0.5f, 0.0f , 0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 1.0f,1.0f } },
        oGFX::Vertex{ {-0.5f, 0.0f , 0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 0.0f,1.0f } },
    };
    std::vector<uint32_t>planeIndices{
        0,2,1,2,0,3
    };


    std::vector<oGFX::Vertex>triVerts{
            oGFX::Vertex{ {-0.5,-0.5,0.0}, { 1.0f,0.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
            oGFX::Vertex{ { 0.5,-0.5,0.0}, { 0.0f,1.0f,0.0f }, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f } },
            oGFX::Vertex{ { 0.0, 0.5,0.0}, { 0.0f,0.0f,1.0f }, { 0.0f,0.0f,1.0f }, { 0.0f,1.0f } }
    };
    std::vector<uint32_t>triIndices{
        0,1,2
    };

    constexpr glm::vec3 redColor = glm::vec3{ 1.0f,0.0f,0.0f };
    constexpr glm::vec3 dirX = glm::vec3{ 1.0f,0.0f,0.0f };
    constexpr glm::vec3 dirY = glm::vec3{ 0.0f,1.0f,0.0f };
    constexpr glm::vec3 dirZ = glm::vec3{ 0.0f,0.0f,1.0f };

    std::vector<oGFX::Vertex> boxverts
    {
        /*
    	oGFX::Vertex{ {-0.5,-0.5,-0.5}, { -1.0f,-1.0f,-1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
    	oGFX::Vertex{ { 0.5,-0.5,-0.5}, { 0.0f,-1.0f,0.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
    	oGFX::Vertex{ {-0.5, 0.5,-0.5}, { -1.0f,0.0f,-1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
    	oGFX::Vertex{ { 0.5, 0.5,-0.5}, { 0.0f,0.0f,-1.0f },{ 1.0f,0.0f,0.0f }, { 1.0f,1.0f } },
    
    	oGFX::Vertex{ {-0.5,-0.5, 0.5}, { -1.0f,-1.0f,0.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
    	oGFX::Vertex{ { 0.5,-0.5, 0.5}, { 1.0f,-1.0f,1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
    	oGFX::Vertex{ {-0.5, 0.5, 0.5}, { -1.0f,1.0f,1.0f },{ 1.0f,0.0f,0.0f }, { 0.0f,1.0f } },
    	oGFX::Vertex{ { 0.5, 0.5, 0.5}, { 1.0f,1.0f,1.0f },{ 1.0f,0.0f,0.0f }, { 1.0f,1.0f } }
        */

        // 
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, dirZ, redColor, { 0, 1 }, dirX },
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, dirZ, redColor, { 1, 1 }, dirX },
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, dirZ, redColor, { 0, 0 }, dirX },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, dirZ, redColor, { 1, 0 }, dirX },
        //
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, -dirZ, redColor, { 0, 1 }, -dirX },
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, -dirZ, redColor, { 1, 1 }, -dirX },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, -dirZ, redColor, { 0, 0 }, -dirX },
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, -dirZ, redColor, { 1, 0 }, -dirX },
        //
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, dirX, redColor, { 0, 1 }, -dirZ },
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, dirX, redColor, { 1, 1 }, -dirZ },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, dirX, redColor, { 0, 0 }, -dirZ },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, dirX, redColor, { 1, 0 }, -dirZ },
        //
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, -dirX, redColor, { 0, 1 }, dirZ },
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, -dirX, redColor, { 1, 1 }, dirZ },
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, -dirX, redColor, { 0, 0 }, dirZ },
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, -dirX, redColor, { 1, 0 }, dirZ },
        //
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, dirY, redColor, { 0, 1 }, dirX },
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, dirY, redColor, { 1, 1 }, dirX },
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, dirY, redColor, { 0, 0 }, dirX },
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, dirY, redColor, { 1, 0 }, dirX },
        //
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, -dirY, redColor, { 0, 1 }, dirX },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, -dirY, redColor, { 1, 1 }, dirX },
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, -dirY, redColor, { 0, 0 }, dirX },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, -dirY, redColor, { 1, 0 }, dirX }

        /*
        // Data here is dumped from my engine somewhere...
        [0]	{position=[-0.5 0.5 0.5] normal=[0 0 1] tangent=[1 0 0] ...}     {bitangent=[0 1 0] uv=[0 1]
        [1]	{position=[0.5 0.5 0.5] normal=[0 0 1] tangent=[1 0 0] ...}      {bitangent=[0 1 0] uv=[1 1]
        [2]	{position=[-0.5 -0.5 0.5] normal=[0 0 1] tangent=[1 0 0] ...}    {bitangent=[0 1 0] uv=[0 0]
        [3]	{position=[0.5 -0.5 0.5] normal=[0 0 1] tangent=[1 0 0] ...}     {bitangent=[0 1 0] uv=[1 0]

        [4]	{position=[0.5 0.5 -0.5] normal=[-8.74228e-08 0 -1] tangent=[-1 0 8.9407e-08] ...}       {bitangent=[0 1 0] uv=[0 1]
        [5]	{position=[-0.5 0.5 -0.5] normal=[-8.74228e-08 0 -1] tangent=[-1 0 8.9407e-08] ...}      {bitangent=[0 1 0] uv=[1 1]
        [6]	{position=[0.5 -0.5 -0.5] normal=[-8.74228e-08 0 -1] tangent=[-1 0 8.9407e-08] ...}      {bitangent=[0 1 0] uv=[0 0]
        [7]	{position=[-0.5 -0.5 -0.5] normal=[-8.74228e-08 0 -1] tangent=[-1 0 8.9407e-08] ...}     {bitangent=[0 1 0] uv=[1 0]

        [8]	{position=[0.5 0.5 0.5] normal=[1 0 -4.37114e-08] tangent=[-2.98023e-08 0 -1] ...}        {bitangent=[0 1 0] uv=[0 1]
        [9]	{position=[0.5 0.5 -0.5] normal=[1 0 -4.37114e-08] tangent=[-2.98023e-08 0 -1] ...}       {bitangent=[0 1 0] uv=[1 1]
        [10	{position=[0.5 -0.5 0.5] normal=[1 0 -4.37114e-08] tangent=[-2.98023e-08 0 -1] ...}       {bitangent=[0 1 0] uv=[0 0]
        [11 {position=[0.5 -0.5 -0.5] normal=[1 0 -4.37114e-08] tangent=[-2.98023e-08 0 -1] ...}      {bitangent=[0 1 0] uv=[1 0]

        [12]	{position=[-0.5 0.5 -0.5] normal=[-1 0 -4.37114e-08] tangent=[-2.98023e-08 0 1] ...}     {bitangent=[0 1 0] uv=[0 1]
        [13]	{position=[-0.5 0.5 0.5] normal=[-1 0 -4.37114e-08] tangent=[-2.98023e-08 0 1] ...}      {bitangent=[0 1 0] uv=[1 1]
        [14]	{position=[-0.5 -0.5 -0.5] normal=[-1 0 -4.37114e-08] tangent=[-2.98023e-08 0 1] ...}    {bitangent=[0 1 0] uv=[0 0]
        [15]	{position=[-0.5 -0.5 0.5] normal=[-1 0 -4.37114e-08] tangent=[-2.98023e-08 0 1] ...}     {bitangent=[0 1 0] uv=[1 0]

        [16]	{position=[-0.5 0.5 -0.5] normal=[0 1 -4.37114e-08] tangent=[1 0 0] ...}   {bitangent=[0 -2.98023e-08 -1] uv=[0 1]
        [17]	{position=[0.5 0.5 -0.5] normal=[0 1 -4.37114e-08] tangent=[1 0 0] ...}    {bitangent=[0 -2.98023e-08 -1] uv=[1 1]
        [18]	{position=[-0.5 0.5 0.5] normal=[0 1 -4.37114e-08] tangent=[1 0 0] ...}    {bitangent=[0 -2.98023e-08 -1] uv=[0 0]
        [19]	{position=[0.5 0.5 0.5] normal=[0 1 -4.37114e-08] tangent=[1 0 0] ...}     {bitangent=[0 -2.98023e-08 -1] uv=[1 0]

        [20]	{position=[-0.5 -0.5 0.5] normal=[0 -1 -4.37114e-08] tangent=[1 0 0] ...}     {bitangent=[0 -2.98023e-08 1] uv=[0 1]
        [21]	{position=[0.5 -0.5 0.5] normal=[0 -1 -4.37114e-08] tangent=[1 0 0] ...}      {bitangent=[0 -2.98023e-08 1] uv=[1 1]
        [22]	{position=[-0.5 -0.5 -0.5] normal=[0 -1 -4.37114e-08] tangent=[1 0 0] ...}    {bitangent=[0 -2.98023e-08 1] uv=[0 0]
        [23]	{position=[0.5 -0.5 -0.5] normal=[0 -1 -4.37114e-08] tangent=[1 0 0] ...}     {bitangent=[0 -2.98023e-08 1] uv=[1 0]
        */
    };
    for (auto& v : boxverts) { v.norm = glm::normalize(v.norm); }

    static std::vector<uint32_t> boxindices
    {
        /*
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
        */
        0,2,1,
        1,2,3,
        4,6,5,
        5,6,7,
        8,10,9,
        9,10,11,
        12,14,13,
        13,14,15,
        16,18,17,
        17,18,19,
        20,22,21,
        21,22,23
    };

    std::vector<Point3D> pts{
        glm::vec3{5.03f,1.34f,3.0f},
        glm::vec3{7.0f,10.0f,10.0f},
        glm::vec3{-5.0f,0.0f,3.5f},
        glm::vec3{6.5f,-3.63f,-5.81f},
        glm::vec3{4.5f,0.0f,5.0f},
        glm::vec3{8.0f,-3.0f,5.0f},
    };
   
    

    //uint32_t yes = renderer.LoadMeshFromFile("Models/TextObj.obj");
    //uint32_t yes = renderer.LoadMeshFromFile("Models/Skull_textured.fbx");
   

    std::unique_ptr<Model> icoSphere{};
    {
        auto [pos,triangleList] = icosahedron::make_icosphere(1);

        std::vector<oGFX::Vertex> vertices;
        vertices.reserve(pos.size());
        for (size_t i = 0; i < pos.size(); i++)
        {
            oGFX::Vertex v{};
            v.pos = pos[i];
            vertices.push_back(v);
        }
        std::vector<uint32_t>indices;
        indices.reserve(triangleList.size() * 3ull);
        for (size_t i = 0; i < triangleList.size(); i++)
        {           
            indices.push_back(triangleList[i].vertex[0]);
            indices.push_back(triangleList[i].vertex[1]);
            indices.push_back(triangleList[i].vertex[2]);

            glm::vec3 e1 = vertices[triangleList[i].vertex[0]].pos - vertices[triangleList[i].vertex[1]].pos;
            glm::vec3 e2 = vertices[triangleList[i].vertex[2]].pos - vertices[triangleList[i].vertex[1]].pos;
            glm::vec3 norm = glm::normalize(glm::cross(e1, e2));
            for (size_t j = 0; j < 3; j++)
            {
                vertices[triangleList[i].vertex[j]].norm = norm;
            }

        }
        renderer.g_MeshBuffers.VtxBuffer.reserve(100000*sizeof(oGFX::Vertex));
        renderer.g_MeshBuffers.IdxBuffer.reserve(100000*sizeof(oGFX::Vertex));
        icoSphere.reset( renderer.LoadMeshFromBuffers(vertices, indices, nullptr));
    }

    std::vector<Point3D> vertPositions;
    std::unique_ptr<Model> bunny { renderer.LoadMeshFromFile("Models/bunny.ply") };
    vertPositions.resize(bunny->vertices.size());
    std::transform(bunny->vertices.begin(), bunny->vertices.end(), vertPositions.begin(), [](const oGFX::Vertex& v) { return v.pos; });
    oGFX::BV::RitterSphere(bunny->s, vertPositions);
    oGFX::BV::BoundingAABB(bunny->aabb, vertPositions);
    Sphere ms;
    //oGFX::BV::RittersEigenSphere(bunny->s, vertPositions);

    vertPositions.resize(icoSphere->vertices.size());
    std::transform(icoSphere->vertices.begin(), icoSphere->vertices.end(), vertPositions.begin(), [](const oGFX::Vertex& v) { return v.pos; });
    oGFX::BV::RitterSphere(icoSphere->s, vertPositions);
    oGFX::BV::BoundingAABB(icoSphere->aabb, vertPositions);

    std::unique_ptr<Model> box{ renderer.LoadMeshFromBuffers(boxverts, boxindices, nullptr) };
    vertPositions.resize(box->vertices.size());
    std::transform(box->vertices.begin(), box->vertices.end(), vertPositions.begin(), [](const oGFX::Vertex& v) { return v.pos; });
    oGFX::BV::RitterSphere(box->s, vertPositions);
    oGFX::BV::BoundingAABB(box->aabb, vertPositions);

    //std::unique_ptr<Model> ball;
    //ball.reset(renderer.LoadMeshFromFile("Models/sphere.obj"));
    
    //int o{};
    //std::vector<glm::vec3> positions(bunny->vertices.size());
    //std::transform(bunny->vertices.begin(), bunny->vertices.end(), positions.begin(), [](const oGFX::Vertex& v) { return v.pos; });
    //for (auto& v : bunny->vertices)
    //{
    //    //std::cout << v.pos << " ";
    //    //if ((++o % 5) == 0) std::cout << std::endl;
    //    //++o;
    //}
    //
    //
    //std::cout << std::endl;
    //std::cout << "vertices : " << bunny->vertices.size() << std::endl;
    //
    //
    //uint32_t triangle = renderer.LoadMeshFromBuffers(quadVerts, quadIndices, nullptr);
    std::unique_ptr<Model> plane{ renderer.LoadMeshFromBuffers(planeVerts, planeIndices, nullptr) };
    //delete bunny;
    //
    //oGFX::BV::RitterSphere(ms, positions);
    //std::cout << "Ritter Sphere " << ms.center << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_6);
    //std::cout << "Larson_06 Sphere " << ms.center << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_14);
    //std::cout << "Larson_14 Sphere " << ms.center << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_26);
    //std::cout << "Larson_26 Sphere " << ms.center << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_98);
    //std::cout << "Larson_98 Sphere " << ms.center << " , r = " << ms.radius << std::endl;
    //std::cout << "Eigen Sphere " << ms.center << " , r = " << ms.radius << std::endl;
    //AABB ab;
    ////positions.resize(ball->vertices.size());
    ////std::transform(ball->vertices.begin(), ball->vertices.end(), positions.begin(), [](const oGFX::Vertex& v) { return v.pos; });
    ////oGFX::BV::BoundingAABB(ab, positions);
    ////std::cout << "AABB " << ab.center << " , Extents = " << ab.halfExt << std::endl;
    //
    //glm::mat4 id(1.0f);



    
    VulkanRenderer::EntityDetails ed;
    ed.name = "Plane";
    ed.modelID = plane->gfxIndex;
    ed.pos = { 0.0f,-2.0f,0.0f };
    ed.scale = { 30.0f,1.0f,30.0f };
    //renderer.entities.push_back(ed);
    ed.name = "Sphere";
    ed.modelID = icoSphere->gfxIndex;
    ed.pos = { -2.0f,0.0f,-2.0f };
    ed.scale = { 1.0f,1.0f,1.0f };
    renderer.entities.push_back(ed);

    ed.modelID = bunny->gfxIndex;
    ed.name = "Bunny";
    ed.pos = { -3.0f,2.0f,-3.0f };
    ed.scale = { 2.0f,1.0f,2.0f };
    renderer.entities.push_back(ed);
    //ed.modelID = triangle;
    //ed.pos = { 0.0f,0.0f,0.0f };  
    //renderer.entities.push_back(ed);
    ed.modelID = box->gfxIndex;
    ed.name = "Box";
    ed.pos = { 2.0f,0.0f,2.0f };
    ed.scale = { 2.0f,3.0f,1.0f };
    renderer.entities.push_back(ed);
    ed.pos = { 5.0f,-1.0f,2.0f };
    ed.scale = { 1.0f,3.0f,3.0f };
    renderer.entities.push_back(ed);
    ed.pos = { 5.0f,0.0f,-5.0f };
    ed.scale = { 2.0f,1.2f,2.0f };
    renderer.entities.push_back(ed);
    ed.pos = { 3.0f,-2.0f,-5.0f };
    ed.scale = { 1.0f,1.2f,2.0f };
    renderer.entities.push_back(ed);
    ed.pos = { 1.0f,-2.0f,5.0f };
    ed.scale = { 1.0f,1.0f,1.0f };
    renderer.entities.push_back(ed);

    //renderer.entities.resize(2);

    for (auto& e: renderer.entities)
    {
        AABB ab;
        auto& model = renderer.models[e.modelID];
        ab.center = e.pos;
        ab.halfExt = e.scale * 0.5f;
        e.aabb = model.cpuModel->aabb;
        e.aabb.center += e.pos;
        e.aabb.halfExt *= e.scale;


        e.sphere = model.cpuModel->s;
        e.sphere.center += e.pos;
        e.sphere.radius *= std::max({ 
            e.scale[0],
            e.scale[1],
            e.scale[2]}) ;
        
        //renderer.AddDebugBox(ab, {(iter+1)%3,(iter+2%3),(iter%3),1});
    }

    std::vector<uint32_t> ids(renderer.entities.size());
    std::iota(ids.begin(), ids.end(), 0);

    Tree<VulkanRenderer::EntityDetails,Sphere> topDownSphere;
    topDownSphere.root = std::make_unique<TreeNode<Sphere>>();
    Tree<VulkanRenderer::EntityDetails, Sphere>::TopDownTree<VulkanRenderer::EntityDetails, Sphere>
        (renderer.entities, topDownSphere.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );

    std::vector<std::pair<uint32_t,Sphere>> topSphereDebugs;
    topDownSphere.getDrawList<VulkanRenderer::EntityDetails, Sphere>(topSphereDebugs,topDownSphere.root.get());

    for (auto& [depth,sphere] : topSphereDebugs)
    {         
        renderer.AddDebugSphere(sphere, oGFX::Colors::c[depth], renderer.g_topDwn_Sphere);
    }

    Tree<VulkanRenderer::EntityDetails,AABB> boxTree;
    std::iota(ids.begin(), ids.end(), 0);
    boxTree.root = std::make_unique<TreeNode<AABB>>();
    Tree<VulkanRenderer::EntityDetails, AABB>::TopDownTree<VulkanRenderer::EntityDetails, AABB>(renderer.entities, boxTree.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );
    std::vector<std::pair<uint32_t,AABB>> topBoxDebugs;
    boxTree.getDrawList<VulkanRenderer::EntityDetails, AABB>(topBoxDebugs,boxTree.root.get());
    for (auto& [depth,box] : topBoxDebugs)
    {
        renderer.AddDebugBox(box, oGFX::Colors::c[depth],renderer.g_topDwn_AABB);
    }


    std::iota(ids.begin(), ids.end(), 0);
    Tree<VulkanRenderer::EntityDetails,AABB> btmTree;
    btmTree.BottomUpTree(renderer.entities, btmTree.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );

    std::vector<std::pair<uint32_t,AABB>> btmBoxDebugs;
    btmTree.getDrawList<VulkanRenderer::EntityDetails, AABB>(btmBoxDebugs,btmTree.root.get());
    for (auto& [depth,box] : btmBoxDebugs)
    {
        renderer.AddDebugBox(box, oGFX::Colors::c[depth],renderer.g_btmUp_AABB);
    }


    std::iota(ids.begin(), ids.end(), 0);
    Tree<VulkanRenderer::EntityDetails,Sphere> btmSphereTree;
    btmSphereTree.BottomUpTree(renderer.entities, btmSphereTree.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );

    std::vector<std::pair<uint32_t,Sphere>> btmSphereDebugs;
    btmSphereTree.getDrawList<VulkanRenderer::EntityDetails, Sphere>(btmSphereDebugs,btmSphereTree.root.get());
    for (auto& [depth,sphere] : btmSphereDebugs)
    {
        renderer.AddDebugSphere(sphere, oGFX::Colors::c[depth],renderer.g_btmUp_Sphere);
    }

    //uint32_t Object = renderer.CreateMeshModel("Models/TextObj.obj");
    //uint32_t obj = renderer.CreateMeshModel(verts, indices);
   //renderer.CreateTexture("Textures/TD_Checker_Base_Color.png");
   //renderer.CreateTexture("TD_Checker_Mixed_AO.png");
   //renderer.CreateTexture("TD_Checker_Normal_OpenGL.png");
   //renderer.CreateTexture("TD_Checker_Roughness.png");

   auto alb = renderer.CreateTexture("Textures/TD_Checker_Base_Color.dds");
   auto norm =renderer.CreateTexture("Textures/TD_Checker_Normal_OpenGL.dds");
   auto occlu =renderer.CreateTexture("Textures/TD_Checker_Mixed_AO.dds");
   auto rough =renderer.CreateTexture("Textures/TD_Checker_Roughness.dds");   


   //create a hundred random textures because why not
   std::default_random_engine rndEngine(123456);
   std::uniform_int_distribution<uint32_t> uniformDist( 0xFF000000, 0xFFFFFFFF );
   std::vector<oGFX::InstanceData> instanceData;
   constexpr size_t numTex = 5;
   constexpr size_t dims = 2;
   std::vector<uint32_t> bitmap(dims*dims);
   for (size_t i = 0; i < numTex; i++)
   {
       for (size_t x = 0; x < bitmap.size(); x++)
       {
       uint32_t colour = uniformDist(rndEngine); // ABGR
        bitmap[x] = colour;
       }
       renderer.CreateTexture(dims, dims, reinterpret_cast<unsigned char*>(bitmap.data()));
   }

   glm::mat4 xform{ 1.0f };
   xform = glm::translate(xform, glm::vec3(-3.0f, 0.0f, -3.0f));
   xform = glm::scale(xform, glm::vec3{ 4.0f,4.0f,4.0f });

    float angle = 0.0f;
    auto lastTime = std::chrono::high_resolution_clock::now();

    renderer.camera.type = Camera::CameraType::lookat;
    renderer.camera.target = glm::vec3(0.01f, 0.0f, 0.0f);
    renderer.camera.SetRotation(glm::vec3(0.0f, 0.0f, 0.0f));
    renderer.camera.SetRotationSpeed(0.5f);
    renderer.camera.SetPosition(glm::vec3(0.1f, 2.0f, 10.5f));
    renderer.camera.movementSpeed = 5.0f;

    bool freezeLight = false;

    //renderer.UpdateDebugBuffers();

    bool geomChanged = false;
    glm::vec3 pos{0.1f, 1.1f, -3.5f};
    // handling winOS messages
    // This will handle inputs and pass it to our input callback
    while( mainWindow.windowShouldClose == false )  // infinite loop
    {
        //reset keys
        Input::Begin();
        while(Window::PollEvents());

        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>( now - lastTime).count();
        lastTime = now;

        renderer.camera.keys.left =     Input::GetKeyHeld(KEY_A)? true : false;
        renderer.camera.keys.right =    Input::GetKeyHeld(KEY_D)? true : false;
        renderer.camera.keys.down =     Input::GetKeyHeld(KEY_S)? true : false;
        renderer.camera.keys.up =       Input::GetKeyHeld(KEY_W)? true : false;
        renderer.camera.Update(deltaTime);


        if (mainWindow.m_width != 0 && mainWindow.m_height != 0)
        {
            renderer.camera.SetPerspective(60.0f, (float)mainWindow.m_width / (float)mainWindow.m_height, 0.1f, 10000.0f);
        }

        auto mousedel = Input::GetMouseDelta();
        float wheelDelta = Input::GetMouseWheel();
        if (Input::GetMouseHeld(MOUSE_RIGHT)) {
            renderer.camera.Rotate(glm::vec3(-mousedel.y * renderer.camera.rotationSpeed, mousedel.x * renderer.camera.rotationSpeed, 0.0f));
        }
        if (renderer.camera.type == Camera::CameraType::lookat)
        {
            renderer.camera.ChangeDistance(wheelDelta * -0.001f);
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

        //std::cout<<  renderer.camera.position << '\n';

        if (geomChanged)
        {
            // update bv
            for (auto& e: renderer.entities)
            {
                AABB ab;
                auto& model = renderer.models[e.modelID];
                ab.center = e.pos;
                ab.halfExt = e.scale * 0.5f;
                e.aabb = model.cpuModel->aabb;
                e.aabb.center += e.pos;
                e.aabb.halfExt *= e.scale ;

                e.sphere = model.cpuModel->s;
                e.sphere.center += e.pos;
                e.sphere.radius *= std::max({ 
                    e.scale[0],
                    e.scale[1],
                    e.scale[2]});
                
            }

            {// TOPDOWN AABB
                std::iota(ids.begin(), ids.end(), 0);
                auto& debug = renderer.g_DebugDraws[renderer.g_topDwn_AABB];
                debug.dirty = true;
                boxTree.Clear();
                boxTree.root = std::make_unique<TreeNode<AABB>>();
                Tree<VulkanRenderer::EntityDetails, AABB>::TopDownTree<VulkanRenderer::EntityDetails, AABB>
                    (renderer.entities, boxTree.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );

                debug.vertex.clear();
                debug.indices.clear();
                topBoxDebugs.clear();
                boxTree.getDrawList<VulkanRenderer::EntityDetails, AABB>(topBoxDebugs, boxTree.root.get());
                for (auto& [depth,box] : topBoxDebugs)
                {
                    renderer.AddDebugBox(box, oGFX::Colors::c[depth],renderer.g_topDwn_AABB);
                }
            }

            {// BOTTOM AABB
                std::iota(ids.begin(), ids.end(), 0);
                auto& debug = renderer.g_DebugDraws[renderer.g_btmUp_AABB];
                debug.dirty = true;
                btmTree.Clear();
                btmTree.BottomUpTree(renderer.entities, btmTree.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );

                debug.vertex.clear();
                debug.indices.clear();

                btmBoxDebugs.clear();
                btmTree.getDrawList<VulkanRenderer::EntityDetails, AABB>(btmBoxDebugs,btmTree.root.get());
                for (auto& [depth,box] : btmBoxDebugs)
                {
                    renderer.AddDebugBox(box, oGFX::Colors::c[depth],renderer.g_btmUp_AABB);
                }
            }
            
            {// TOP DOWN SPHERE
                std::iota(ids.begin(), ids.end(), 0);
                auto& debug = renderer.g_DebugDraws[renderer.g_topDwn_Sphere];
                debug.dirty = true;
                topDownSphere.Clear();
                topDownSphere.root = std::make_unique<TreeNode<Sphere>>();
                Tree<VulkanRenderer::EntityDetails, Sphere>::TopDownTree<VulkanRenderer::EntityDetails, Sphere>
                    (renderer.entities, topDownSphere.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );
                debug.vertex.clear();
                debug.indices.clear();

                topSphereDebugs.clear();
                topDownSphere.getDrawList<VulkanRenderer::EntityDetails, Sphere>(topSphereDebugs,topDownSphere.root.get());
                for (auto& [depth,sphere] : topSphereDebugs)
                {         
                    renderer.AddDebugSphere(sphere, oGFX::Colors::c[depth], renderer.g_topDwn_Sphere);
                }
            }

            {// BOTTOMUP SPHERE
                std::iota(ids.begin(), ids.end(), 0);
                auto& debug = renderer.g_DebugDraws[renderer.g_btmUp_Sphere];
                debug.dirty = true;
                btmSphereTree.Clear();
                btmSphereTree.BottomUpTree(renderer.entities, btmSphereTree.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );
                debug.vertex.clear();
                debug.indices.clear();

                btmSphereDebugs.clear();
                btmSphereTree.getDrawList<VulkanRenderer::EntityDetails, Sphere>(btmSphereDebugs,btmSphereTree.root.get());
                for (auto& [depth,sphere] : btmSphereDebugs)
                {
                    renderer.AddDebugSphere(sphere, oGFX::Colors::c[depth],renderer.g_btmUp_Sphere);
                }
            }

           //std::iota(ids.begin(), ids.end(), 0);
           //btmTree.Clear();
           //renderer.g_debugDrawVerts.clear();
           //renderer.g_debugDrawIndices.clear();
           //btmTree.BottomUpTree(renderer.entities, btmTree.root.get(), ids.data(), static_cast<uint32_t>(ids.size()) );
           //
           //std::vector<std::pair<uint32_t,AABB>> btmDebugs;
           //btmTree.getDrawList<VulkanRenderer::EntityDetails, AABB>(btmDebugs, btmTree.root.get());
           //for (auto& [depth,box] : btmDebugs)
           //{
           //    renderer.AddDebugBox(box, oGFX::Colors::c[depth]);
           //}           

            //renderer.UpdateDebugBuffers();
            geomChanged = false;
        }

        renderer.UpdateTreeBuffers();

        if (renderer.gpuTransformBuffer.MustUpdate())
        {
            auto dbi = renderer.gpuTransformBuffer.GetDescriptorBufferInfo();
            //VkWriteDescriptorSet write = oGFX::vk::inits::writeDescriptorSet(renderer.g0_descriptors, 
            //    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
            //    3,
            //    &dbi);
            //vkUpdateDescriptorSets(renderer.m_device.logicalDevice, 1, &write, 0, 0);
            DescriptorBuilder::Begin(&VulkanRenderer::DescLayoutCache, &VulkanRenderer::DescAlloc)
                .BindBuffer(3, &dbi, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
                .Build(VulkanRenderer::g0_descriptors,VulkanRenderer::g0_descriptorsLayout);
            renderer.gpuTransformBuffer.Updated();
        }

        if (renderer.PrepareFrame() == true)
        {
            
            renderer.timer += deltaTime*0.25f;
            renderer.UpdateLightBuffer();
            renderer.Draw();
            renderer.PrePass();
            //renderer.SimplePass();
            
            //renderer.RecordCommands(renderer.swapchainImageIndex);
            
            if (renderer.deferredRendering)
            {
                renderer.DeferredPass();
                renderer.DeferredComposition();
            }
            renderer.DebugPass();

            // Create a dockspace over the mainviewport so that we can dock stuff
            ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), 
                ImGuiDockNodeFlags_PassthruCentralNode // make the dockspace transparent
                | ImGuiDockNodeFlags_NoDockingInCentralNode // dont allow docking in the central area
            );
            
            //ImGui::ShowDemoWindow();
            
            ImGui::Begin("Entity List");
            int id{};
            
            for (auto& entity: renderer.entities)
            {
                ImGui::PushID(id++);
                
                ImGui::BulletText("[%d] ", entity.modelID);
                ImGui::SameLine();
                ImGui::Text(entity.name.c_str());
                
                geomChanged |= ImGui::DragFloat3("Position", glm::value_ptr(entity.pos), 0.01f);
                geomChanged |= ImGui::DragFloat3("Scale", glm::value_ptr(entity.scale), 0.01f);
                geomChanged |= ImGui::DragFloat("theta", &entity.rot);
                geomChanged |= ImGui::DragFloat3("Rotation Vec", glm::value_ptr(entity.rotVec));

                ImGui::PopID();
            }
            ImGui::End();

            ImGui::Begin("Debug Draws");

            static const char* sphereTypes[]
            {
                "Hello"
            };

            //ImGui::ListBox()
            ImGui::Checkbox("Top down AABB", &renderer.debug_btmUp_aabb);
            ImGui::Checkbox("Bottom Up AABB", &renderer.debug_topDown_aabb);
            ImGui::Checkbox("Top down Sphere", &renderer.debug_topDown_sphere);
            ImGui::Checkbox("Bottom Up Sphere", &renderer.debug_btmUp_sphere);
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
