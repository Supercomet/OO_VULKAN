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
	
	// Flip y during sample
    uvs = vec2(uvs.x, 1.0 - uvs.y);
	
	// Bounds check for the actual shadow map
    float sampledDepth = 0.0;
    float lowerBoundsLimit = 0.000001;
    float boundsLimit = 0.999999;
    if (projCoords.x > boundsLimit || projCoords.x < lowerBoundsLimit
		|| projCoords.y > boundsLimit || projCoords.y < lowerBoundsLimit
		)
    {
        return 1.0;
    }
    else
    {
        sampledDepth = texture(

        sampler2D( samplerShadows, basicSampler),
        uvs).
        r;
    }
    float currDepth = projCoords.z;

    float maxbias = PC.maxBias;
    float mulBias = PC.mulBias;
    float bias = max(mulBias * (1.0 - NdotL), maxbias);
    float shadow = 1.0;
    if (currDepth < sampledDepth - bias)
    {
        if (currDepth > 0 && currDepth < 1.0)
        {
            shadow = 0.20;
        }
    }

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
