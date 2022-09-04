#pragma once

#include "MathCommon.h"
#include "../shaders/shared_structs.h"
#include "BitContainer.h"

#include <vector>
#include <array>

enum ObjectInstanceFlags : uint32_t // fuck enum class
{
    STATIC_INSTANCE  = 0x1,  // Object will never change after initialization
    DYNAMIC_INSTANCE = 0x2,  // Object is dynamic (spatial/property)
    ACTIVE_FLAG      = 0x4,  // Object is inactive, skip for all render pass
    SHADOW_CASTER    = 0x8,  // Object casts shadows (put it into shadow render pass)
    SHADOW_RECEIVER  = 0x10, // Object receives shadows (a mask for lighting pass)
    ENABLE_ZPREPASS  = 0x20, // Object is added to Z-Prepass
                             // etc

};

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
    
    ObjectInstanceFlags flags;

    uint32_t modelID{}; // Index for the mesh
    uint32_t entityID{}; // Unique ID for this entity instance
};

struct DecalInstance
{

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

    // TODO: Fix Me !
    std::array<OmniLightInstance, 6> m_HardcodedOmniLights;

private:
    BitContainer<ObjectInstance> m_ObjectInstances;
    BitContainer<OmniLightInstance> m_OmniLightInstances;
    //etc

    // + Spatial Acceleration Structures
    // + Culling object BV against frustum
};
