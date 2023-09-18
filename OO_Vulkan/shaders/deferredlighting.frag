layout (location = 0) in vec2 inUV;
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
	
	// Get G-Buffer values
	vec4 depth = texture(sampler2D(samplerDepth,basicSampler), inUV);

	vec4 albedo = texture(sampler2D(samplerAlbedo,basicSampler), inUV);
	float ambient = PC.ambient;
	vec3 fragPos = WorldPosFromDepth(depth.r,inUV,uboFrameContext.inverseProjection,uboFrameContext.inverseView);
	vec3 normal = texture(sampler2D(samplerNormal,basicSampler), inUV).rgb;
	normal = normalize(normal);

	const float gamma = 2.2;
	albedo.rgb =  pow(albedo.rgb, vec3(1.0/gamma));

	vec4 material = texture(sampler2D(samplerMaterial,basicSampler), inUV);
	float SSAO = texture(sampler2D(samplerSSAO,basicSampler), inUV).r;
	float roughness = 1.0 - clamp(material.r,0.005,1.0);
	float metalness = clamp(material.g,0.005,1.0);

	// just perform ambient
	vec3 lightDir = -normalize(PC.directionalLight.xyz);
	
	// Ambient part
	vec3 emissive = texture(sampler2D(samplerEmissive,basicSampler),inUV).rgb;
	emissive = vec3(0);

	vec4 lightCol = vec4(1.0,1.0,1.0,1.0);
	lightCol.w = PC.ambient; 
	vec3 result = EvalDirectionalLight(lightCol, lightDir,fragPos,normal,roughness,albedo.rgb,metalness);

	result += 0.2 * albedo.rgb;

	//result = albedo.rgb  * ambient + emissive;
	outFragcolor = vec4(result.rgb, albedo.a);	

}
