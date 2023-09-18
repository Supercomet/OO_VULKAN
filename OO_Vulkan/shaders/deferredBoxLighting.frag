layout (location = 1) in flat int inLightInstance;
layout (location = 0) out vec4 outFragcolor;

#include "frame.shader"
layout(set = 1, binding = 0) uniform UboFrameContext
{
    FrameContext uboFrameContext;
};
#include "shared_structs.h"

layout (set = 0, binding = 0) uniform sampler basicSampler; 
layout (set = 0, binding = 1) uniform texture2D samplerDepth;
layout (set = 0, binding = 2) uniform texture2D samplerNormal;
layout (set = 0, binding = 3) uniform texture2D samplerAlbedo;
layout (set = 0, binding = 4) uniform texture2D samplerMaterial;
layout (set = 0, binding = 5) uniform texture2D samplerEmissive;
layout (set = 0, binding = 6) uniform texture2D samplerShadows;
layout (set = 0, binding = 7) uniform texture2D samplerSSAO;

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
	vec4 depth = texture(sampler2D(samplerDepth,basicSampler), inUV);
	vec3 fragPos = WorldPosFromDepth(depth.r,inUV,uboFrameContext.inverseProjection,uboFrameContext.inverseView);

	outFragcolor = vec4(0,0,0,1);
	vec3 normal = texture(sampler2D(samplerNormal,basicSampler), inUV).rgb;
	if(dot(normal,normal) == 0.0) return;
	normal = normalize(normal);

	vec4 albedo = texture(sampler2D(samplerAlbedo,basicSampler), inUV);
	const float gamma = 2.2;
	albedo.rgb =  pow(albedo.rgb, vec3(1.0/gamma));

	vec4 material = texture(sampler2D(samplerMaterial,basicSampler), inUV);
	float SSAO = texture(sampler2D(samplerSSAO,basicSampler), inUV).r;
	float roughness = 1.0 - clamp(material.r,0.005,1.0);
	float metalness = clamp(material.g,0.005,1.0);


	// remove SSAO if not wanted
	if(PC.useSSAO == 0){
		SSAO = 1.0;
	}
	
	float outshadow = texture(sampler2D(samplerShadows,basicSampler),inUV).r;
	
	LocalLightInstance lightInfo = Lights_SSBO[inLightInstance];

	vec3 lightContribution = vec3(0.0);
	vec3 res = EvalLight(lightInfo, fragPos, normal, roughness ,albedo.rgb, metalness);	
	lightContribution += res;
	
	// calculate shadow if this is a shadow light
	float shadowValue = EvalShadowMap(lightInfo, inLightInstance, normal, fragPos);
	lightContribution *= shadowValue;

	float ambient = 0.0;
	ambient = PC.ambient;
	vec3 ambientContribution = albedo.rgb  * ambient;
	ambientContribution = vec3(0.0);

	//vec3 emissive = texture(sampler2D(samplerEmissive,basicSampler),inUV).rgb;
	vec3 emissive = vec3(0);
	vec3 result =  (ambientContribution * SSAO + lightContribution) + emissive;

	outFragcolor = vec4(result, albedo.a);	

}
