layout (location = 1) in flat int inLightInstance;
layout (location = 0) out vec4 outFragcolor;

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
layout (set = 0, binding = 6) uniform texture2D textureShadows;
layout (set = 0, binding = 7) uniform texture2D textureSSAO;
// layout 8 taken by buffer
layout (set = 0, binding = 9) uniform sampler cubeSampler;
layout (set = 0, binding = 10) uniform textureCube irradianceCube;
layout (set = 0, binding = 11)uniform textureCube prefilterCube;
layout (set = 0, binding = 12)uniform texture2D brdfLUT;

layout (set = 0, binding = 13)uniform samplerShadow shadowSampler;
layout (set = 0, binding = 14)uniform sampler clampedSampler;
layout (set = 0, binding = 15)uniform texture2D LTC;
layout (set = 0, binding = 16)uniform texture2D LTCLUT;


#include "lights.shader"

layout( push_constant ) uniform lightpc
{
	LightPC PC;
};

#include "shadowCalculation.shader"
#include "lightingEquations.shader"


// area lights

const float LUT_SIZE = 64.0; // ltc_texture size
const float LUT_SCALE = (LUT_SIZE - 1.0) / LUT_SIZE;
const float LUT_BIAS = 0.5 / LUT_SIZE;
// Vector form without project to the plane (dot with the normal)
// Use for proxy sphere clipping
vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    // Using built-in acos() function will result flaws
    // Using fitting result for calculating acos()
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
    float b = 3.4175940 + (4.1616724 + y) * y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5 * inversesqrt(max(1.0 - x * x, 1e-7)) - v;

    return cross(v1, v2) * theta_sintheta;
}

// P is fragPos in world space (LTC distribution)
vec3 LTC_Evaluate(// texture2D inLut,
sampler samplerIn, //should work but doesnt
vec3 N, vec3 V, vec3 P
, mat3 Minv, vec3 points[4], bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = Minv * transpose(mat3(T1, T2, N));
	//Minv = Minv * transpose(mat3(N, T2, T1));

    // polygon (allocate 4 vertices for clipping)
    vec3 L[4];
    // transform polygon from LTC back to origin Do (cosine weighted)
    L[0] = Minv * (points[0] - P);
    L[1] = Minv * (points[1] - P);
    L[2] = Minv * (points[2] - P);
    L[3] = Minv * (points[3] - P);

    // use tabulated horizon-clipped sphere
    // check if the shading point is behind the light
    vec3 dir = points[0] - P; // LTC space
    vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
    bool behind = (dot(dir, lightNormal) > 0.0);

    // cos weighted space
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

	// integrate
    vec3 vsum = vec3(0.0);
    vsum += IntegrateEdgeVec(L[1],L[0]);
    vsum += IntegrateEdgeVec(L[2],L[1]);
    vsum += IntegrateEdgeVec(L[3],L[2]);
    vsum += IntegrateEdgeVec(L[0],L[3]);

    // form factor of the polygon in direction vsum
    float len = length(vsum);

    float z = vsum.z / len;
    if (behind)
        z = -z;

    vec2 uv = vec2(z * 0.5f + 0.5f, len); // range [0, 1]
    uv = uv * LUT_SCALE + LUT_BIAS;

    // Fetch the form factor for horizon clipping
    //float scale = textureLod(sampler2D(inLut,samplerIn), uv, 0).w;
    float scale = textureLod(sampler2D(LTCLUT,samplerIn), uv, 0).w;

    float sum = len * scale;
    if (!behind && !twoSided)
        sum = 0.0;

    // Outgoing radiance (solid angle) for the entire polygon
    vec3 Lo_i = vec3(sum, sum, sum);
    return Lo_i;
}



uint DecodeFlags(in float value)
{
    return uint(value * 255.0f);
}

#include "shader_utility.shader"

void main()
{

	vec2 inUV = gl_FragCoord.xy / PC.resolution.xy;
	
	// Get G-Buffer values
	vec4 depth = texture(sampler2D(textureDepth,basicSampler), inUV);
    vec3 fragWorldPos = WorldPosFromDepth(depth.r, inUV, uboFrameContext.inverseProjectionJittered, uboFrameContext.inverseView);

	outFragcolor = vec4(0,0,0,1);
	vec3 normal = DecodeNormalHelper(texture(sampler2D(textureNormal,basicSampler), inUV).rgb);
	if(dot(normal,normal) == 0.0) return;
	normal = normalize(normal);

	vec4 albedo = texture(sampler2D(textureAlbedo,basicSampler), inUV);
    albedo.rgb = GammaToLinear(albedo.rgb);

	vec4 material = texture(sampler2D(textureMaterial,basicSampler), inUV);
	float SSAO = texture(sampler2D(textureSSAO,basicSampler), inUV).r;
	float roughness = material.r;
	float metalness = material.g;
	
	//vec3 emissive = texture(sampler2D(samplerEmissive,basicSampler),inUV).rgb;
    vec3 emissive = vec3(0);

	// remove SSAO if not wanted
	if(PC.useSSAO == 0){
		SSAO = 1.0;
	}
	
	float outshadow = texture(sampler2D(textureShadows,basicSampler),inUV).r;
	
	LocalLightInstance lightInfo = Lights_SSBO[inLightInstance];

	vec3 lightContribution = vec3(0.0);
    vec3 res = EvalLight(lightInfo, fragWorldPos, uboFrameContext.cameraPosition.xyz, normal, roughness, albedo.rgb, metalness);
	lightContribution += res;
	
	// calculate shadow if this is a shadow light
	
    vec3 lightDir = lightInfo.position.xyz - fragWorldPos;
	

    SurfaceProperties surface;
    surface.albedo = albedo.rgb;
    surface.roughness = roughness;
    surface.metalness = metalness;
    surface.lightCol = lightInfo.color.rgb * lightInfo.color.w;
    surface.lightRadius = lightInfo.radius.x;
    surface.N = normalize(normal);
    surface.V = normalize(uboFrameContext.cameraPosition.xyz - fragWorldPos);
    surface.L = normalize(lightDir);
    surface.H = normalize(surface.L + surface.V);
    surface.dist = length(lightDir);
	
    float attenuation = UnrealFalloff(surface.dist, surface.lightRadius);
    //surface.lightCol *= attenuation;
	
    vec3 irradiance = vec3(1);
    irradiance = texture(samplerCube(irradianceCube, basicSampler), normal).rgb;
    irradiance = vec3(0);
    vec3 R = normalize(reflect(-surface.V, surface.N));
	
    const float MAX_REFLECTION_LOD = 6.0;
    vec3 prefilteredColor = textureLod(samplerCube(prefilterCube, basicSampler), R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 lutVal = texture(sampler2D( brdfLUT, basicSampler), vec2(max(dot(surface.N, surface.V), 0.0), roughness)).rg;

	prefilteredColor = vec3(0);
    vec3 result = vec3(0);
	
    switch (lightInfo.info.w)
    {
        case 1: // point lights
        {
                result = SaschaWillemsDirectionalLight(surface,
													irradiance,
													prefilteredColor,
													lutVal);
        }
        break;
        case 2: // area lights
		{
            // do area light caluclations
            float NoV = max(dot(surface.N, surface.V), 0.0);
            vec2 roughnessUV = vec2(surface.roughness, sqrt(1.0f - NoV));
            roughnessUV = roughnessUV * LUT_SCALE + LUT_BIAS;
            
            // get 4 parameters for inverse_M
            vec4 t1 = texture(sampler2D(LTC,clampedSampler), roughnessUV);

            // Get 2 parameters for Fresnel calculation
            vec4 t2 =texture(sampler2D(LTCLUT,clampedSampler), roughnessUV);

            mat3 Minv = mat3(
                vec3(t1.x, 0, t1.y),
                vec3(0, 1, 0),
                vec3(t1.z, 0, t1.w)
            );

            vec3 vertices[4] = vec3[4](
                vec3(-1.0, -1.0, 0.0),  // Bottom left
                vec3(1.0, -1.0, 0.0),   // Bottom right
                vec3(-1.0, 1.0, 0.0),   // Top left
                vec3(1.0, 1.0, 0.0)     // Top right
            );
            
            LightInfo decodedLight = DecodeLightInfo(lightInfo);
            
            
            // Evaluate LTC shading
                vec3 diffuse  = LTC_Evaluate( /*LTCLUT,*/clampedSampler, surface.N, surface.V, fragWorldPos, mat3(1), decodedLight.rectPoints, false);
                vec3 specular = LTC_Evaluate(/*LTCLUT,*/ clampedSampler, surface.N, surface.V, fragWorldPos, Minv, decodedLight.rectPoints, false);
                
		    // GGX BRDF shadowing and Fresnel
		    // t2.x: shadowedF90 (F90 normally it should be 1.0)
		    // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L, H).
            // specular *= mSpecular * t2.x + (1.0f - mSpecular) * t2.y;

                specular *= surface.roughness * t2.x + (1.0f - surface.roughness) * t2.y;
		    // Add contribution
                result += surface.lightCol * (specular + surface.albedo.rgb * diffuse);
            }
        break;
        default:
            result = vec3(0);

    }   
	
    float shadowValue = EvalShadowMap(lightInfo, inLightInstance, normal, fragWorldPos);
 
    result *= shadowValue;
    result *= attenuation;
	
	outFragcolor = vec4(result, albedo.a);	

}
