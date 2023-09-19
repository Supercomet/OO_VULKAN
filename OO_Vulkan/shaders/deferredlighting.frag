layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;

 #extension GL_EXT_samplerless_texture_functions : require


#include "frame.shader"
layout(set = 1, binding = 0) uniform UboFrameContext
{
    FrameContext uboFrameContext;
};
#include "shared_structs.h"

layout (set = 0, binding = 0) uniform sampler basicSampler; 
layout (set = 0, binding = 1) uniform texture2D textureDepth;
layout (set = 0, binding = 2) uniform texture2D textureNormal;
layout (set = 0, binding = 3) uniform texture2D textureAlbedo;
layout (set = 0, binding = 4) uniform texture2D textureMaterial;
layout (set = 0, binding = 5) uniform texture2D textureEmissive;
layout (set = 0, binding = 6) uniform texture2D samplerShadows;
layout (set = 0, binding = 7) uniform texture2D textureSSAO;

layout (set = 0, binding = 9) uniform sampler cubeSampler;
layout (set = 0, binding = 10) uniform textureCube skyTexture;


#include "lights.shader"

layout( push_constant ) uniform lightpc
{
	LightPC PC;
};

#include "shadowCalculation.shader"
#include "lightingEquations.shader"


uint DecodeFlags(in float value)
{
    return uint(value * 255.0f);
}

#include "shader_utility.shader"

void main()
{
	
	// Get G-Buffer values
	float depth = texelFetch(textureDepth,ivec2(gl_FragCoord.xy) , 0).r;

	vec4 albedo = texture(sampler2D(textureAlbedo,basicSampler), inUV);
	float ambient = PC.ambient;
	vec3 fragPos = WorldPosFromDepth(depth.r,inUV,uboFrameContext.inverseProjection,uboFrameContext.inverseView);
	vec3 normal = texture(sampler2D(textureNormal,basicSampler), inUV).rgb;
	normal = normalize(normal);

	const float gamma = 2.2;
	albedo.rgb =  pow(albedo.rgb, vec3(1.0/gamma));
	
    //albedo.rgb = vec3(1,1,0);

	vec4 material = texture(sampler2D(textureMaterial,basicSampler), inUV);
	float SSAO = texture(sampler2D(textureSSAO,basicSampler), inUV).r;
    float roughness = (material.r);
	float metalness = (material.g);

	// just perform ambient
	vec3 lightDir = -normalize(PC.directionalLight.xyz);
	
	// Ambient part
	vec3 emissive = texture(sampler2D(textureEmissive,basicSampler),inUV).rgb;
	emissive = vec3(0);

	vec4 lightCol = vec4(1.0,1.0,1.0,1.0);
	lightCol.w = PC.ambient; 
    
	
    vec3 result = EvalDirectionalLight(lightCol, lightDir, uboFrameContext.cameraPosition.xyz, fragPos, normal, roughness, albedo.rgb, metalness);
	
    vec3 cubeCol = texture(samplerCube(skyTexture, basicSampler), normal).rgb;
	
    outFragcolor = vec4(result.rgb, albedo.a);

}
