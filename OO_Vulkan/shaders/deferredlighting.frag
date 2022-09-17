layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;

#include "frame.shader"
layout(set = 1, binding = 0) uniform UboFrameContext
{
    FrameContext uboFrameContext;
};

layout (set = 0, binding = 1) uniform sampler2D samplerposition;
layout (set = 0, binding = 2) uniform sampler2D samplerNormal;
layout (set = 0, binding = 3) uniform sampler2D samplerAlbedo;
layout (set = 0, binding = 4) uniform sampler2D samplerMaterial;
layout (set = 0, binding = 5) uniform sampler2D samplerDepth;

#include "shared_structs.h"

layout (binding = 6) uniform UBO
{
	OmniLightInstance lights[6];
	vec4 viewPos;
	int displayDebugTarget;
} ubo;

const int lightCount = 6;

vec3 CalculatePointLight_NonPBR(int lightIndex, in vec3 fragPos, in vec3 normal, in vec3 albedo, in float specular)
{
	vec3 result = vec3(0.0f, 0.0f, 0.0f);
	
	// Vector to light
	vec3 L = ubo.lights[lightIndex].position.xyz - fragPos;
	// Distance from light to fragment position
	float dist = length(L);
	
	// Viewer to fragment
	vec3 V = ubo.viewPos.xyz - fragPos;
	V = normalize(V);
	
	//if(dist < ubo.lights[lightIndex].radius)
	{
		// Light to fragment
		L = normalize(L);
	
		// Attenuation
		float atten = ubo.lights[lightIndex].radius.x / (pow(dist, 2.0) + 1.0);
	
		// Diffuse part
		vec3 N = normalize(normal);
		float NdotL = max(0.0, dot(N, L));
		vec3 diff = ubo.lights[lightIndex].color.xyz * albedo.rgb * NdotL * atten;
	
		// Specular part
		// Specular map values are stored in alpha of albedo mrt
		//vec3 R = -reflect(L, N);
		//float RdotV = max(0.0, dot(R, V));
		//vec3 spec = ubo.lights[lightIndex].color.xyz * specular * pow(RdotV, 16.0) * atten;
	
		result = diff;// + spec;	
	}

	return result;
}

uint DecodeFlags(in float value)
{
    return uint(value * 255.0f);
}

void main()
{
	// Get G-Buffer values
	vec3 fragPos = texture(samplerposition, inUV).rgb;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);
	vec4 material = texture(samplerMaterial, inUV);

	// Render-target composition

	float ambient = 0.5f;
	if (DecodeFlags(material.z) == 0x1)
	{
		ambient = 1.0f;
	}

	// Ambient part
	vec3 result = albedo.rgb * ambient;
	
	// Point Lights
	for(int i = 0; i < lightCount; ++i)
	{
		result += CalculatePointLight_NonPBR(i, fragPos, normal, albedo.rgb, albedo.a);
	}    	
   
	outFragcolor = vec4(result, 1.0);	
}
