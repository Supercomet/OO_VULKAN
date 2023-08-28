layout (location = 0) in vec2 inUVo2;
layout (location = 1) in flat int inLightInstance;
layout (location = 0) out vec4 outFragcolor;

#include "frame.shader"
layout(set = 1, binding = 0) uniform UboFrameContext
{
    FrameContext uboFrameContext;
};
#include "shared_structs.h"

layout (set = 0, binding = 1) uniform sampler2D samplerDepth;
// layout (set = 0, binding = 1) uniform sampler2D samplerposition; we construct position using depth
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;
layout (set = 0, binding = 3) uniform sampler2D samplerAlbedo;
layout (set = 0, binding = 4) uniform sampler2D samplerMaterial;
layout (set = 0, binding = 5) uniform sampler2D samplerEmissive;
layout (set = 0, binding = 6) uniform sampler2D samplerShadows;
layout (set = 0, binding = 7) uniform sampler2D samplerSSAO;

#include "lights.shader"

layout( push_constant ) uniform lightpc
{
	LightPC PC;
};

#include "lightingEquations.shader"


uint DecodeFlags(in float value)
{
    return uint(value * 255.0f);
}

#include "shader_utility.shader"

void main()
{

	vec2 inUV = gl_FragCoord.xy / PC.resolution.xy;

	
	// Get G-Buffer values
	vec4 depth = texture(samplerDepth, inUV);
	vec3 fragPos = WorldPosFromDepth(depth.r,inUV,uboFrameContext.inverseProjection,uboFrameContext.inverseView);
	//fragPos.z = depth.r;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	if(dot(normal,normal) == 0.0)
	{
		outFragcolor = vec4(0);
	//	outFragcolor = vec4(0.0,0.0,1.0,1.0);
		return;
	}
	vec4 albedo = texture(samplerAlbedo, inUV);
	vec4 material = texture(samplerMaterial, inUV);
	float SSAO = texture(samplerSSAO, inUV).r;
	float specular = material.g;
	float roughness = 1.0 - material.r;

	// Render-target composition
	//float ambient = PC.ambient;
	float ambient = 0.0;
	//if (DecodeFlags(material.z) == 0x1)
	//{
	//	ambient = 1.0;
	//}
	
	const float gamma = 2.2;
	albedo.rgb =  pow(albedo.rgb, vec3(1.0/gamma));

	// Ambient part
	vec3 result = albedo.rgb  * ambient;

	// remove SSAO if not wanted
	if(PC.useSSAO == 0){
		SSAO = 1.0;
	}
	
	float outshadow = texture(samplerShadows,inUV).r;
	
	// Point Lights
	vec3 lightContribution = vec3(0.0);
	/////////for(int i = 0; i < PC.numLights; ++i)
	{
		vec3 res = EvalLight(inLightInstance, fragPos, normal, roughness ,albedo.rgb, specular);	

		lightContribution += res;
	}

	//lightContribution *= outshadow;
	
	vec3 ambientContribution = albedo.rgb  * ambient;
	//vec3 emissive = texture(samplerEmissive,inUV).rgb;
	vec3 emissive = vec3(0);
	result =  (ambientContribution * SSAO + lightContribution) + emissive;

	outFragcolor = vec4(result, albedo.a);	

}
