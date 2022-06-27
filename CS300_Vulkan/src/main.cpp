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

// Just a dummy struct to hold Vertex and 32-bit Index Buffers.
struct DefaultMesh
{
    std::vector<oGFX::Vertex> m_VertexBuffer;
    std::vector<uint32_t> m_IndexBuffer;
};

DefaultMesh CreateDefaultCubeMesh()
{
    DefaultMesh mesh;

    constexpr glm::vec3 redColor = glm::vec3{ 1.0f,0.0f,0.0f };
    constexpr glm::vec3 dirX = glm::vec3{ 1.0f,0.0f,0.0f };
    constexpr glm::vec3 dirY = glm::vec3{ 0.0f,1.0f,0.0f };
    constexpr glm::vec3 dirZ = glm::vec3{ 0.0f,0.0f,1.0f };

    // The default box mesh must have 6 faces (or rather 6 planes),
    // with the normals correctly pointing outwards relative to the planes.
    // This cube is also unit and normalized (in [-0.5,0.5] range)
    mesh.m_VertexBuffer = 
    {
        // 
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, dirZ, redColor, { 0.0f, 1.0f }, dirX },
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, dirZ, redColor, { 1.0f, 1.0f }, dirX },
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, dirZ, redColor, { 0.0f, 0.0f }, dirX },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, dirZ, redColor, { 1.0f, 0.0f }, dirX },
        //
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, -dirZ, redColor, { 0.0f, 1.0f }, -dirX },
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, -dirZ, redColor, { 1.0f, 1.0f }, -dirX },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, -dirZ, redColor, { 0.0f, 0.0f }, -dirX },
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, -dirZ, redColor, { 1.0f, 0.0f }, -dirX },
        //
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, dirX, redColor, { 0.0f, 1.0f }, -dirZ },
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, dirX, redColor, { 1.0f, 1.0f }, -dirZ },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, dirX, redColor, { 0.0f, 0.0f }, -dirZ },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, dirX, redColor, { 1.0f, 0.0f }, -dirZ },
        //
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, -dirX, redColor, { 0.0f, 1.0f }, dirZ },
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, -dirX, redColor, { 1.0f, 1.0f }, dirZ },
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, -dirX, redColor, { 0.0f, 0.0f }, dirZ },
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, -dirX, redColor, { 1.0f, 0.0f }, dirZ },
        //
        oGFX::Vertex{ {-0.5, 0.5,-0.5}, dirY, redColor, { 0.0f, 1.0f }, dirX },
        oGFX::Vertex{ { 0.5, 0.5,-0.5}, dirY, redColor, { 1.0f, 1.0f }, dirX },
        oGFX::Vertex{ {-0.5, 0.5, 0.5}, dirY, redColor, { 0.0f, 0.0f }, dirX },
        oGFX::Vertex{ { 0.5, 0.5, 0.5}, dirY, redColor, { 1.0f, 0.0f }, dirX },
        //
        oGFX::Vertex{ {-0.5,-0.5, 0.5}, -dirY, redColor, { 0.0f, 1.0f }, dirX },
        oGFX::Vertex{ { 0.5,-0.5, 0.5}, -dirY, redColor, { 1.0f, 1.0f }, dirX },
        oGFX::Vertex{ {-0.5,-0.5,-0.5}, -dirY, redColor, { 0.0f, 0.0f }, dirX },
        oGFX::Vertex{ { 0.5,-0.5,-0.5}, -dirY, redColor, { 1.0f, 0.0f }, dirX }

        // Data here is dumped from my engine somewhere...
        // Putting all these here for sanity check
        //[0]	position=[-0.5  0.5 0.5] normal=[0 0 1] tangent=[1 0 0] bitangent=[0 1 0] uv=[0 1]
        //[1]	position=[ 0.5  0.5 0.5] normal=[0 0 1] tangent=[1 0 0] bitangent=[0 1 0] uv=[1 1]
        //[2]	position=[-0.5 -0.5 0.5] normal=[0 0 1] tangent=[1 0 0] bitangent=[0 1 0] uv=[0 0]
        //[3]	position=[ 0.5 -0.5 0.5] normal=[0 0 1] tangent=[1 0 0] bitangent=[0 1 0] uv=[1 0]
        //
        //[4]	position=[ 0.5  0.5 -0.5] normal=[0 0 -1] tangent=[-1 0 0] bitangent=[0 1 0] uv=[0 1]
        //[5]	position=[-0.5  0.5 -0.5] normal=[0 0 -1] tangent=[-1 0 0] bitangent=[0 1 0] uv=[1 1]
        //[6]	position=[ 0.5 -0.5 -0.5] normal=[0 0 -1] tangent=[-1 0 0] bitangent=[0 1 0] uv=[0 0]
        //[7]	position=[-0.5 -0.5 -0.5] normal=[0 0 -1] tangent=[-1 0 0] bitangent=[0 1 0] uv=[1 0]
        //
        //[8]	position=[0.5  0.5  0.5] normal=[1 0 0] tangent=[0 0 -1] bitangent=[0 1 0] uv=[0 1]
        //[9]	position=[0.5  0.5 -0.5] normal=[1 0 0] tangent=[0 0 -1] bitangent=[0 1 0] uv=[1 1]
        //[10]  position=[0.5 -0.5  0.5] normal=[1 0 0] tangent=[0 0 -1] bitangent=[0 1 0] uv=[0 0]
        //[11]  position=[0.5 -0.5 -0.5] normal=[1 0 0] tangent=[0 0 -1] bitangent=[0 1 0] uv=[1 0]
        //
        //[12]  position=[-0.5  0.5 -0.5] normal=[-1 0 0] tangent=[0 0 1] bitangent=[0 1 0] uv=[0 1]
        //[13]  position=[-0.5  0.5  0.5] normal=[-1 0 0] tangent=[0 0 1] bitangent=[0 1 0] uv=[1 1]
        //[14]  position=[-0.5 -0.5 -0.5] normal=[-1 0 0] tangent=[0 0 1] bitangent=[0 1 0] uv=[0 0]
        //[15]  position=[-0.5 -0.5  0.5] normal=[-1 0 0] tangent=[0 0 1] bitangent=[0 1 0] uv=[1 0]
        //
        //[16]  {position=[-0.5 0.5 -0.5] normal=[0 1 0] tangent=[1 0 0] bitangent=[0 0 -1] uv=[0 1]
        //[17]  {position=[ 0.5 0.5 -0.5] normal=[0 1 0] tangent=[1 0 0] bitangent=[0 0 -1] uv=[1 1]
        //[18]  {position=[-0.5 0.5  0.5] normal=[0 1 0] tangent=[1 0 0] bitangent=[0 0 -1] uv=[0 0]
        //[19]  {position=[ 0.5 0.5  0.5] normal=[0 1 0] tangent=[1 0 0] bitangent=[0 0 -1] uv=[1 0]
        //
        //[20]  {position=[-0.5 -0.5  0.5] normal=[0 -1 0] tangent=[1 0 0] bitangent=[0 0 1] uv=[0 1]
        //[21]  {position=[ 0.5 -0.5  0.5] normal=[0 -1 0] tangent=[1 0 0] bitangent=[0 0 1] uv=[1 1]
        //[22]  {position=[-0.5 -0.5 -0.5] normal=[0 -1 0] tangent=[1 0 0] bitangent=[0 0 1] uv=[0 0]
        //[23]  {position=[ 0.5 -0.5 -0.5] normal=[0 -1 0] tangent=[1 0 0] bitangent=[0 0 1] uv=[1 0]
    };

    mesh.m_IndexBuffer = 
    {
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

    return mesh;
}

// We must be explicit by saying this is a XZ-plane, because a plane can mean anything, is it XY, XZ, or YZ...?
DefaultMesh CreateDefaultPlaneXZMesh()
{
    DefaultMesh mesh;
    
    mesh.m_VertexBuffer =
    {
        oGFX::Vertex{ {-0.5f, 0.0f ,-0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
        oGFX::Vertex{ { 0.5f, 0.0f ,-0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 1.0f,0.0f } },
        oGFX::Vertex{ { 0.5f, 0.0f , 0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 1.0f,1.0f } },
        oGFX::Vertex{ {-0.5f, 0.0f , 0.5f}, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 0.0f,1.0f } },
    };

    mesh.m_IndexBuffer = 
    {
        0,2,1,
        2,0,3
    };

    return mesh;
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

    std::vector<oGFX::Vertex>quadVerts
    {
            oGFX::Vertex{ {-0.5,-0.5,0.0}, { 1.0f,0.0f,0.0f }, { 1.0f,0.0f,0.0f }, { 0.0f,0.0f } },
            oGFX::Vertex{ { 0.5,-0.5,0.0}, { 0.0f,1.0f,0.0f }, { 0.0f,1.0f,0.0f }, { 1.0f,0.0f } },
            oGFX::Vertex{ { 0.0, 0.5,0.0}, { 0.0f,0.0f,1.0f }, { 0.0f,0.0f,1.0f }, { 0.0f,1.0f } }
    };
    std::vector<uint32_t>quadIndices
    {
        0,1,2
    };

    std::vector<Point3D> pts
    {
        glm::vec3{5.03f,1.34f,3.0f},
        glm::vec3{7.0f,10.0f,10.0f},
        glm::vec3{-5.0f,0.0f,3.5f},
        glm::vec3{6.5f,-3.63f,-5.81f},
        glm::vec3{4.5f,0.0f,5.0f},
        glm::vec3{8.0f,-3.0f,5.0f},
    };

    //uint32_t yes = renderer.LoadMeshFromFile("Models/TextObj.obj");
    //uint32_t yes = renderer.LoadMeshFromFile("Models/Skull_textured.fbx");

    
    auto defaultPlaneMesh = CreateDefaultPlaneXZMesh();
    auto defaultCubeMesh = CreateDefaultCubeMesh();
    uint32_t planeMeshIndex = renderer.LoadMeshFromBuffers(defaultPlaneMesh.m_VertexBuffer, defaultPlaneMesh.m_IndexBuffer, nullptr);
    uint32_t cubeMeshIndex = renderer.LoadMeshFromBuffers(defaultCubeMesh.m_VertexBuffer, defaultCubeMesh.m_IndexBuffer, nullptr);
    renderer.m_MeshIndexToMeshName.emplace(planeMeshIndex, "Plane");
    renderer.m_MeshIndexToMeshName.emplace(cubeMeshIndex, "Cube");

    uint32_t icoSphereMeshIndex{};
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
        uint32_t max{};
        uint32_t min{};
        for (size_t i = 0; i < triangleList.size(); i++)
        {
            max = std::max<uint32_t>({ max,
                (uint32_t)triangleList[i].vertex[0],
                (uint32_t)triangleList[i].vertex[2],
                (uint32_t)triangleList[i].vertex[1] });

            min = std::min<uint32_t>({ min,
                (uint32_t)triangleList[i].vertex[0],
                (uint32_t)triangleList[i].vertex[2],
                (uint32_t)triangleList[i].vertex[1] });

            indices.push_back(triangleList[i].vertex[0]);
            indices.push_back(triangleList[i].vertex[1]);
            indices.push_back(triangleList[i].vertex[2]);
        }
        std::cout << "max:" << max << " min: " << min << " verts " << vertices.size() << std::endl;
        renderer.g_MeshBuffers.VtxBuffer.reserve(100000*sizeof(oGFX::Vertex));
        renderer.g_MeshBuffers.IdxBuffer.reserve(100000*sizeof(oGFX::Vertex));
        icoSphereMeshIndex = renderer.LoadMeshFromBuffers(vertices, indices, nullptr);
        renderer.m_MeshIndexToMeshName.emplace(icoSphereMeshIndex, "Sphere");
    }
    //uint32_t box = renderer.LoadMeshFromBuffers(boxverts, boxindices, nullptr);
    

    //std::unique_ptr<Model> ball;
    //ball.reset(renderer.LoadMeshFromFile("Models/sphere.obj"));
    //auto bunny = renderer.LoadMeshFromFile("Models/bunny.ply");
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
    //uint32_t plane = renderer.LoadMeshFromBuffers(planeVerts, planeIndices, nullptr);
    
    //delete bunny;
    //
    //Sphere ms;
    //oGFX::BV::RitterSphere(ms, positions);
    //std::cout << "Ritter Sphere " << ms.centre << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_6);
    //std::cout << "Larson_06 Sphere " << ms.centre << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_14);
    //std::cout << "Larson_14 Sphere " << ms.centre << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_26);
    //std::cout << "Larson_26 Sphere " << ms.centre << " , r = " << ms.radius << std::endl;
    //oGFX::BV::LarsonSphere(ms, positions, oGFX::BV::EPOS::_98);
    //std::cout << "Larson_98 Sphere " << ms.centre << " , r = " << ms.radius << std::endl;
    //oGFX::BV::RittersEigenSphere(ms, positions);
    //std::cout << "Eigen Sphere " << ms.centre << " , r = " << ms.radius << std::endl;
    //AABB ab;
    ////positions.resize(ball->vertices.size());
    ////std::transform(ball->vertices.begin(), ball->vertices.end(), positions.begin(), [](const oGFX::Vertex& v) { return v.pos; });
    ////oGFX::BV::BoundingAABB(ab, positions);
    ////std::cout << "AABB " << ab.center << " , Extents = " << ab.halfExt << std::endl;
    //
    //glm::mat4 id(1.0f);

    // Temporary solution to assign random numbers... Shamelessly stolen from Intel.
    auto FastRandomMagic = []() -> uint32_t
    {
        static uint32_t seed = 0xDEADBEEF;
        seed = (214013 * seed + 2531011);
        return (seed >> 16) & 0x7FFF;
    };

    // Setup some default objects in the scene
    {
        VulkanRenderer::EntityDetails entity;
        entity.pos = { 0.0f,-2.0f,0.0f };
        entity.scale = { 30.0f,30.0f,30.0f };
        entity.modelID = planeMeshIndex;
        entity.entityID = FastRandomMagic();
        renderer.entities.push_back(entity);

        entity.pos = { -2.0f,0.0f,-2.0f };
        entity.scale = { 1.0f,1.0f,1.0f };
        entity.modelID = icoSphereMeshIndex;
        entity.entityID = FastRandomMagic();
        renderer.entities.push_back(entity);

        entity.pos = { 2.0f,0.0f,2.0f };
        entity.scale = { 1.0f,1.0f,1.0f };
        entity.modelID = cubeMeshIndex;
        entity.entityID = FastRandomMagic();
        renderer.entities.push_back(entity);

        //entity.modelID = triangle;
        //entity.pos = { 0.0f,0.0f,0.0f };  
        //renderer.entities.push_back(entity);
    }
    
    int iter = 0;
    for (auto& e: renderer.entities)
    {
        AABB ab;
        ab.center = e.pos;
        ab.halfExt = e.scale * 0.5f;
        renderer.AddDebugBox(ab, {(iter+1)%3,(iter+2%3),(iter%3),1});
        ++iter;
    }

    
    auto [sphVertices, spfIndices] = icosahedron::make_icosphere(2);
    auto currsz = renderer.g_debugDrawVerts.size();
    renderer.g_debugDrawVerts.reserve(renderer.g_debugDrawVerts.size() + sphVertices.size());
    for (auto&& v : sphVertices)
    {
        renderer.g_debugDrawVerts.emplace_back(oGFX::Vertex{ v });
    }
    
    renderer.g_debugDrawIndices.reserve( renderer.g_debugDrawIndices.size() + spfIndices.size()*3);
    for (auto&& ind : spfIndices) 
    {
        renderer.g_debugDrawIndices.emplace_back(ind.vertex[0]+static_cast<uint32_t>(currsz));
        renderer.g_debugDrawIndices.emplace_back(ind.vertex[1]+static_cast<uint32_t>(currsz));
        renderer.g_debugDrawIndices.emplace_back(ind.vertex[0]+static_cast<uint32_t>(currsz)); 
        renderer.g_debugDrawIndices.emplace_back(ind.vertex[2]+static_cast<uint32_t>(currsz));
        renderer.g_debugDrawIndices.emplace_back(ind.vertex[2]+static_cast<uint32_t>(currsz));
        renderer.g_debugDrawIndices.emplace_back(ind.vertex[1]+static_cast<uint32_t>(currsz));
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
   //renderer.SetMeshTextures(yes, alb, norm, occlu, rough);

   //renderer.SubmitMesh(yes, position);


   


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

    static bool freezeLight = false;

  

    //for (size_t i = 0; i < boxindices.size()-1; i++)
    //{
    //    ab.center + Point3D{ab.halfExt.x, ab.halfExt.y,ab.halfExt.z};
    //    ab.center + Point3D{-ab.halfExt.x, ab.halfExt.y,ab.halfExt.z};
    //    ab.center + Point3D{-ab.halfExt.x, ab.halfExt.y,ab.halfExt.z};
    //}
    //renderer.g_debugDrawIndices.push_back(boxindices[boxindices.size()-1]);
    //renderer.g_debugDrawIndices.push_back(boxindices[0]);

   

    renderer.UpdateDebugBuffers();


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

        renderer.camera.keys.left = Input::GetKeyHeld(KEY_A)? true : false;
        renderer.camera.keys.right = Input::GetKeyHeld(KEY_D)? true : false;
        renderer.camera.keys.down = Input::GetKeyHeld(KEY_S)? true : false;
        renderer.camera.keys.up = Input::GetKeyHeld(KEY_W)? true : false;
        renderer.camera.Update(deltaTime);
        renderer.camera.movementSpeed = 5.0f;
        float speed = 0.05f;

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
            
            renderer.timer += deltaTime * 0.25f;
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
            
            // Display ImGui Window
            {
                if (ImGui::Begin("Scene Helper"))
                {
                    if (ImGui::BeginTabBar("SceneHelperTabBar"))
                    {
                        if (ImGui::BeginTabItem("Entity"))
                        {
                            if (ImGui::SmallButton("Create Cube"))
                            {
                                VulkanRenderer::EntityDetails entity;
                                entity.pos = { 2.0f,2.0f,2.0f };
                                entity.scale = { 1.0f,1.0f,1.0f };
                                entity.modelID = cubeMeshIndex;
                                entity.entityID = FastRandomMagic();
                                renderer.entities.push_back(entity);
                            }

                            for (auto& entity : renderer.entities)
                            {
                                ImGui::PushID(entity.modelID);

                                ImGui::BulletText("[ID:%u] ", entity.entityID);
                                ImGui::SameLine();

                                // TODO: This code could not be made cleaner... (this is seen as a temporary solution)
                                auto iter = renderer.m_MeshIndexToMeshName.find(entity.modelID);
                                if (iter != renderer.m_MeshIndexToMeshName.end())
                                    ImGui::Text("%s", iter->second ? iter->second : "(nullptr)");
                                else
                                    ImGui::Text("(unknown mesh name)");

                                ImGui::DragFloat3("Position", glm::value_ptr(entity.pos), 0.01f);
                                ImGui::DragFloat3("Scale", glm::value_ptr(entity.scale), 0.01f);
                                ImGui::DragFloat3("Rotation Axis", glm::value_ptr(entity.rotVec));
                                ImGui::DragFloat("Theta", &entity.rot);
                                // TODO: We should be using quaternions.........

                                ImGui::PopID();
                            }
                            
                            ImGui::EndTabItem();
                        }//ImGui::BeginTabItem
                        
                        if (ImGui::BeginTabItem("Light"))
                        {
                            ImGui::BeginDisabled(); // TODO remove once implemented
                            if (ImGui::SmallButton("Create PointLight")) {} // TODO Implement me!
                            ImGui::EndDisabled(); // TODO remove once implemented

                            ImGui::Checkbox("Freeze Lights", &freezeLight);
                            ImGui::DragFloat4("ViewPos", glm::value_ptr(renderer.lightUBO.viewPos));
                            for (int i = 0; i < 6; ++i)
                            {
                                ImGui::PushID(i);
                                auto& light = renderer.lightUBO.lights[i];
                                ImGui::DragFloat4("Position", glm::value_ptr(light.position));
                                ImGui::DragFloat3("Color", glm::value_ptr(light.color));
                                ImGui::DragFloat("Radius", &light.radius);
                                ImGui::PopID();
                            }

                            ImGui::EndTabItem();
                        }//ImGui::BeginTabItem

                        if (ImGui::BeginTabItem("Settings"))
                        {
                            // TODO?
                            ImGui::EndTabItem();
                        }//ImGui::BeginTabItem

                        ImGui::EndTabBar();
                    }//ImGui::BeginTabBar
                }//ImGui::Begin
                ImGui::End();
            }

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
