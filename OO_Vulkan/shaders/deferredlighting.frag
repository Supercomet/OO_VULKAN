layout (location = 0) in vec2 inUV;
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
layout (set = 0, binding = 5) uniform sampler2D samplerShadows;
layout (set = 0, binding = 6) uniform sampler2D samplerSSAO;


layout(std430, set = 0, binding = 7) readonly buffer Lights
{
	SpotLightInstance Lights_SSBO[];
};

layout( push_constant ) uniform lightpc
{
	LightPC PC;
};

#include "lightingEquations.shader"

vec2 GetShadowMapRegion(int lightIndex, in vec2 uv, in vec2 gridSize)
{
	int grid = Lights_SSBO[lightIndex].info.y;
	vec2 gridIncrement = vec2(1.0)/gridSize; // size for each cell

	vec2 actualUV = gridIncrement * uv; // uv local to this cell

	int y = grid/int(gridSize.x);
	int x = grid - int(gridSize.x*y);

	vec2 offset = gridIncrement * vec2(x,y); // offset to our cell

	return offset+actualUV; //sampled position
}

float ShadowCalculation(int lightIndex, in vec4 fragPosLightSpace, float NdotL)
{

	// perspective divide
	vec4 projCoords = fragPosLightSpace/fragPosLightSpace.w;
	//normalization [0,1] tex coords only.. FOR VULKAN DONT DO Z
	projCoords.xy = projCoords.xy* 0.5 + 0.5;

	float maxbias =  PC.maxBias;
	float mulBias = PC.mulBias;
	float bias = max(mulBias * (1.0 - NdotL),maxbias);
	// Flip y during sample
	vec2 uvs = vec2(projCoords.x,1.0-projCoords.y);
	uvs = GetShadowMapRegion(int(lightIndex),uvs,PC.shadowMapGridDim);
	//uvs = vec2(uvs.x, 1.0-uvs.y);
	


	// TODO: add more textures
	float closestDepth = texture(samplerShadows,uvs).r;
	float currDepth = projCoords.z;

	float shadow = 1.0;
	if (projCoords.w > 0.0 && currDepth - bias > closestDepth ) 
	{
		if(projCoords.z < 1)
		{
			shadow = 0.25;		
		}
	}
	//shadow = currDepth - bias > closestDepth ? 1.0 : 0.0;

	return shadow;
}

vec3 EvalLight(int lightIndex, in vec3 fragPos, in vec3 normal,float roughness, in vec3 albedo, float specular, out float shadow)
{
	vec3 result = vec3(0.0f, 0.0f, 0.0f);
	vec3 N = normalize(normal);
	float alpha = roughness;
	vec3 Kd = albedo;
	vec3 Ks = vec3(specular);
	
	// Vector to light
	vec3 L = Lights_SSBO[lightIndex].position.xyz - fragPos;

	// Distance from light to fragment position
	float dist = length(L);
	
	// Viewer to fragment
	vec3 V = uboFrameContext.cameraPosition.xyz - fragPos;
	
	// Light to fragment
	L = normalize(L);	
	V = normalize(V);
	vec3 H = normalize(L+V);
	float NdotL = max(0.0, dot(N, L));

	//if(dist < Lights_SSBO[lightIndex].radius.x)
	{
		//SpotLightInstance light = SpotLightInstance(Omni_LightSSBO[lightIndex]); 
	    
		float r1 = Lights_SSBO[lightIndex].radius.x * 0.9;
		float r2 = Lights_SSBO[lightIndex].radius.x;
		vec3 lCol = Lights_SSBO[lightIndex].color.xyz;

    		// Attenuation
		float atten = Lights_SSBO[lightIndex].radius.x / (pow(dist, 2.0) + 1.0);		 
	
		// Diffuse part
		
		vec3 diff = lCol * GGXBRDF(L , V , H , N , alpha , Kd , Ks) * NdotL * atten;


		// Specular part
		// Specular map values are stored in alpha of albedo mrt
		vec3 R = -reflect(L, N);
		float RdotV = max(0.0, dot(R, V));
		//vec3 spec = lCol * specular * pow(RdotV, 16.0) * atten;
		vec3 spec = lCol * specular * pow(RdotV, 16.0) * atten;
	
		//result = diff;// + spec;	
		result = diff+spec;
	}

	// calculate shadow if this is a shadow light
	//if(Lights_SSBO[lightIndex].position.w < 0)
	if(Lights_SSBO[lightIndex].info.x > 0)
	{
		vec4 outFragmentLightPos = Lights_SSBO[lightIndex].projection * Lights_SSBO[lightIndex].view * vec4(fragPos,1.0);
		shadow = ShadowCalculation(lightIndex,outFragmentLightPos,NdotL);
		
		//result *= shadow;
	}

	return result;
//	return fragPos;
}


uint DecodeFlags(in float value)
{
    return uint(value * 255.0f);
}

#include "shader_utility.shader"

void main()
{
	// Get G-Buffer values
	vec4 depth = texture(samplerDepth, inUV);
	vec3 fragPos = WorldPosFromDepth(depth.r,inUV,uboFrameContext.inverseProjection,uboFrameContext.inverseView);
	//fragPos.z = depth.r;
	vec3 normal = texture(samplerNormal, inUV).rgb;
	vec4 albedo = texture(samplerAlbedo, inUV);
	vec4 material = texture(samplerMaterial, inUV);
	float SSAO = texture(samplerSSAO, inUV).r;
	float specular = material.g;
	float roughness = material.r;

	// Render-target composition
	float ambient = PC.ambient;
	if (DecodeFlags(material.z) == 0x1)
	{
		ambient = 1.0;
	}
	float gamma = 2.2;
	
	albedo.rgb =  pow(albedo.rgb, vec3(gamma));

	// Ambient part
	vec3 result = albedo.rgb  * ambient;

	if(PC.useSSAO != 0){
		result *=  SSAO;
	}
	
	float accShadow = 1.0;
	float outshadow = 1.0;
	// Point Lights
	for(int i = 0; i < PC.numLights; ++i)
	{
		result += EvalLight(i, fragPos, normal, roughness ,albedo.rgb, specular, outshadow);
		result *= outshadow;
	}
	//result*= accShadow;

	result = pow(result, vec3(1.0/gamma));
	if(uboFrameContext.vector4_values0.x<0)
	{
		result = vec3(texture(samplerShadows,inUV).r);
		result = result.r<1.0? vec3(0):result;
	}
	else if(uboFrameContext.vector4_values0.y > 0)
	{
		vec2 uvs = vec2(inUV.x, inUV.y);
		vec2 newUV = GetShadowMapRegion(int(uboFrameContext.vector4_values0.x),uvs,PC.shadowMapGridDim);
		//newUV = vec2(newUV.x,1.0-newUV.y);
		result = vec3(newUV,0.0);
		result = vec3(texture(samplerShadows,newUV).r);
		result = result.r<1.0? vec3(0):result;
	}

	outFragcolor = vec4(result, albedo.a);	
}
