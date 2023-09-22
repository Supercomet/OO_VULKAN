#extension GL_GOOGLE_include_directive : enable
#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;

#include "frame.shader"
layout(set = 1, binding = 0) uniform UboFrameContext
{
    FrameContext uboFrameContext;
};

#include "shared_structs.h"

layout (set = 0, binding = 0) uniform sampler basicSampler; 
layout (set = 0, binding = 1) uniform textureCube   skyTexture; 

#include "shader_utility.shader"

void main()
{
	// Get G-Buffer values
	vec3 worldPos = WorldPosFromDepth(0.1,inUV,uboFrameContext.inverseProjection,uboFrameContext.inverseView);
	vec3 uvw = normalize(worldPos- uboFrameContext.cameraPosition.xyz);
	

    outFragcolor = texture(samplerCube(skyTexture, basicSampler), uvw);
    outFragcolor *= uboFrameContext.vector4_values0.x;
	vec3 gamma= vec3(2.2);
	
    outFragcolor.rgb = (outFragcolor.rgb);
}
