#include "shader_utility.shader"
#include "shared_structs.h"

vec2 GetShadowMapRegion(int gridID, in vec2 uv, in vec2 gridSize)
{
	
    vec2 gridIncrement = vec2(1.0) / gridSize; // size for each cell

    vec2 actualUV = gridIncrement * uv; // uv local to this cell

	// avoid the modolus operator not sure how much that matters
    int y = gridID / int(gridSize.x);
    int x = gridID - int(gridSize.x * y);

    vec2 offset = gridIncrement * vec2(x, y); // offset to our cell

    return offset + actualUV; //sampled position
}

float ShadowCalculation(int lightIndex, int gridID, in vec4 fragPosLightSpace, float NdotL)
{

	// perspective divide
    vec4 projCoords = fragPosLightSpace / fragPosLightSpace.w;
	//normalization [0,1] tex coords only.. FOR VULKAN DONT DO Z
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    vec2 uvs = vec2(projCoords.x, projCoords.y);
    uvs = GetShadowMapRegion(gridID, uvs, PC.shadowMapGridDim);
    
    float oneTexelUV = 1.0/4096;
	
	// Flip y during sample
    uvs = vec2(uvs.x, 1.0 - uvs.y);
	
    float currDepth = projCoords.z;

    float maxbias = PC.maxBias;
    float mulBias = PC.mulBias;
    float bias = max(mulBias * (1.0 - NdotL), maxbias);
    float shadow = 1.0;
    
	// Bounds check for the actual shadow map
    float sampledDepth = 0.0;
    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;
    
    float lowerBoundsLimit = 0.0 + EPSILON;
    float boundsLimit = 1.0 - EPSILON;
    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            if (projCoords.x > boundsLimit || projCoords.x < lowerBoundsLimit
		    || projCoords.y > boundsLimit || projCoords.y < lowerBoundsLimit
		    )
            {
                shadowFactor += 1.0;
            }
            else
            {        
                shadowFactor += texture(sampler2DShadow(textureShadows, shadowSampler), vec3(uvs + vec2(x,y), currDepth + bias)).r;
               
            }
            //shadowFactor += texture(sampler2DShadow(textureShadows, shadowSampler), vec3(uvs, currDepth + bias)).r;
            count++;
        }
	
    }
    shadow = shadowFactor / count;   
   
     //sampledDepth = texture(sampler2D(textureShadows, basicSampler), uvs).r;
    //if (currDepth < sampledDepth - bias)
    //{
    //    if (currDepth > 0 && currDepth < 1.0)
    //    {
    //        shadow = 0.20;
    //    }
    //}
    shadow = clamp(shadow, 0.2, 1.0);

    return shadow;
}

float EvalShadowMap(in LocalLightInstance lightInfo, int lightIndex, in vec3 normal, in vec3 fragPos)
{
    vec3 N = normal;
    vec3 L = normalize(lightInfo.position.xyz - fragPos);
    float NdotL = max(0.0, dot(N, L));
    float shadow = 1.0;
    if (lightInfo.info.x > 0)
    {
        if (lightInfo.info.x == 1)
        {
            int gridID = lightInfo.info.y;
            for (int i = 0; i < 6; ++i)
            {
                vec4 outFragmentLightPos = lightInfo.projection * lightInfo.view[i] * vec4(fragPos, 1.0);
                shadow *= ShadowCalculation(lightIndex, gridID + i, outFragmentLightPos, NdotL);
            }
        }
    }
    return shadow;
}
