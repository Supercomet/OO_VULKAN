#pragma once

#include "MathCommon.h"
#include "../shaders/shared_structs.h"

#include <vector>


struct ObjectInstance
{
    glm::mat4x4 localToWorld{ 1.0f };
};

struct DecalInstance
{

};

enum class ObjectInstanceFlags : uint32_t
{
    STATIC_INSTANCE  = 0x1,  // Object will never change after initialization
    DYNAMIC_INSTANCE = 0x2,  // Object is dynamic (spatial/property)
    ACTIVE_FLAG      = 0x4,  // Object is inactive, skip for all render pass
    SHADOW_CASTER    = 0x8,  // Object casts shadows (put it into shadow render pass)
    SHADOW_RECEIVER  = 0x10, // Object receives shadows (a mask for lighting pass)
    ENABLE_ZPREPASS  = 0x20, // Object is added to Z-Prepass
    // etc
};

// TODO: Move all object storage here...
class GraphicsWorld
{
public:
    auto& GetAllObjectInstances() { return m_ObjectInstances; }
    auto& GetAllOmniLightInstances() { return m_OmniLightInstances; }

private:
    std::vector<ObjectInstance> m_ObjectInstances;
    std::vector<OmniLightInstance> m_OmniLightInstances;
    //etc

    // + Spatial Acceleration Structures
};
