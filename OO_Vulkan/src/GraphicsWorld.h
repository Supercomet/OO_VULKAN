/************************************************************************************//*!
\file           GraphicsWorld.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Declares graphics world, a wrapper for objects that require to be rendered.
    This is used as the main tnerface between the renderer and external engine

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once

#include "MathCommon.h"
#include "../shaders/shared_structs.h"
#include "BitContainer.h"
#include "MeshModel.h"
#include "Camera.h"
#include "VulkanTexture.h"

#include "imgui/imgui.h"
#include <vector>
#include <array>

// pos windows
#undef TRANSPARENT 
enum ObjectInstanceFlags : uint32_t // fuck enum class
{
    STATIC_INSTANCE  = 0x1,  // Object will never change after initialization
    DYNAMIC_INSTANCE = 0x2,  // Object is dynamic (spatial/property)
    ACTIVE_FLAG      = 0x4,  // Object is inactive, skip for all render pass
    SHADOW_CASTER    = 0x8,  // Object casts shadows (put it into shadow render pass)
    SHADOW_RECEIVER  = 0x10, // Object receives shadows (a mask for lighting pass)
    ENABLE_ZPREPASS  = 0x20, // Object is added to Z-Prepass
    TRANSPARENT      = 0x40, // Object is added to forward pass
    EMITTER          = 0x80, // Object is an emitter ??
    SKINNED          = 0x100, // Object is added to skinned pass
                             // etc
};

inline ObjectInstanceFlags operator|(ObjectInstanceFlags a, ObjectInstanceFlags b)
{
    return static_cast<ObjectInstanceFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline ObjectInstanceFlags operator&(ObjectInstanceFlags a, ObjectInstanceFlags b)
{
    return static_cast<ObjectInstanceFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline ObjectInstanceFlags operator~(ObjectInstanceFlags a)
{
    return static_cast<ObjectInstanceFlags>(~static_cast<uint32_t>(a));
}

//CHAR_BIT * sizeof(uint64_t)
struct ObjectInstance
{
    std::string name;
    // Begin These are temp until its fully integrated
    glm::vec3 position{};
    glm::vec3 scale{1.0f};
    float rot{};
    glm::vec3 rotVec{0.0f,1.0f,0.0f};

    uint32_t bindlessGlobalTextureIndex_Albedo{ 0xFFFFFFFF };
    uint32_t bindlessGlobalTextureIndex_Normal{ 0xFFFFFFFF };
    uint32_t bindlessGlobalTextureIndex_Roughness{ 0xFFFFFFFF };
    uint32_t bindlessGlobalTextureIndex_Metallic{ 0xFFFFFFFF };
    // End temp stuff

    uint8_t instanceData{ 0 }; // Per Instance unique data (not to be in material)
    glm::mat4x4 localToWorld{ 1.0f };
    ObjectInstanceFlags flags{static_cast<ObjectInstanceFlags>(SHADOW_RECEIVER | SHADOW_CASTER)};

    // helper functions
    void SetShadowCaster(bool s);
    void SetShadowReciever(bool s);
    void SetSkinned(bool s);

    std::vector<glm::mat4> bones;

    uint32_t modelID{}; // Index for the mesh
    std::bitset<MAX_SUBMESH>submesh;// submeshes to draw
    uint32_t entityID{}; // Unique ID for this entity instance
};



struct DecalInstance
{
    glm::mat4x4 decalViewProjection{ 1.0f };
    glm::vec3 position{ 0.0f, 0.0f, 0.0f };
    glm::vec3 direction{ 0.0f, -1.0f, 0.0f };
    float rotation{ 0.0f };
    float projectorSize{ 1.0f };
    float testVar0{ 1.0f };
    float testVar1{ 1.0f };
    float nearZ{ -1.0f };
};

// TODO: Move all object storage here...
class GraphicsWorld
{
public:
    
    // Call this at the beginning of the frame
    void BeginFrame();
    // Call this at the end of the frame
    void EndFrame();

    auto& GetAllObjectInstances() { return m_ObjectInstances; }
    auto& GetAllOmniLightInstances() { return m_OmniLightInstances; }

    int32_t CreateObjectInstance();
    int32_t CreateObjectInstance(ObjectInstance obj);
    ObjectInstance& GetObjectInstance(int32_t id);
    void DestroyObjectInstance(int32_t id);
    void ClearObjectInstances();

    int32_t CreateLightInstance();
    int32_t CreateLightInstance(OmniLightInstance obj);
    OmniLightInstance& GetLightInstance(int32_t id);
    void DestroyLightInstance(int32_t id);
    void ClearLightInstances();

    uint32_t numCameras = 1;
    std::array<Camera, 2>cameras;
    std::array<int32_t, 2>targetIDs{ -1,-1 };
    std::array<ImTextureID, 2>imguiID{};

    // TODO: Fix Me ! This is for testing
    DecalInstance m_HardcodedDecalInstance;

    struct SSAOSettings
    {
        float radius = 0.5f;
        float bias = 0.025f;
    }ssaoSettings{};

    struct LightingSettings
    {
        float ambient = 0.2f;
        float maxBias = 0.0001f;
        float biasMultiplier = 0.002f;
    }lightSettings{};

    friend class VulkanRenderer;
private:
    int32_t m_entityCount{};
    BitContainer<ObjectInstance> m_ObjectInstances;
    int32_t m_lightCount{};
    BitContainer<OmniLightInstance> m_OmniLightInstances;
    bool initialized = false;
    //etc

    // + Spatial Acceleration Structures
    // + Culling object BV against frustum
};
