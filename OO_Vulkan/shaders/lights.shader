#include "shared_structs.h"

struct LightInfo
{
    bool isShadowCaster;
    int shadowGridIndex;
    int lightType;
    vec4 position;
    vec3 color;
    float intensity;
    vec3 rectPoints[4];
    bool twoSided;
    float radius;
    float innerRadius;
    mat4 projection;
    mat4 view[6];
};


layout(std430, set = 0, binding = 8) readonly buffer Lights
{
	LocalLightInstance Lights_SSBO[];
};


LightInfo DecodeLightInfo(LocalLightInstance l)
{
    LightInfo r;
    r.isShadowCaster = l.info.x > 1;
    r.shadowGridIndex = l.info.y;
    r.lightType = l.info.w;
    r.position = l.position;
    r.color = l.color.rgb;
    r.intensity = l.color.w;
   
    r.twoSided = l.radius.z > 0.0;
    r.radius = l.radius.x;
    r.innerRadius   /*FILL*/;
    r.projection = l.projection;
    
    for (int i = 0; i < 6; ++i)
    {
        r.view[i] = l.view[i];
    }
    
    r.rectPoints[0] = l.view[0][0].xyz;
    r.rectPoints[1] = l.view[0][1].xyz;
    r.rectPoints[2] = l.view[0][3].xyz;
    r.rectPoints[3] = l.view[0][2].xyz;
    
    return r;
}