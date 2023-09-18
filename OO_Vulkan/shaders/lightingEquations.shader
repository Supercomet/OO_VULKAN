#include "shader_utility.shader"
#include "shared_structs.h"

float approx_tan(vec3 V, vec3 N)
{
    float VN = clamp(dot(V,N),EPSILON,1.0);
    return sqrt( 1.0-pow(VN,2) )/VN;
}

float G_SOSS_Approximaton(vec3 L, vec3 H)
{
    return 1.0/ pow(dot(L,H),2);
}

float G_Phong_Beckman_impl(float a)
{
    if(a < 1.6) return 1.0;

    float aa= pow(a,2);
    return (3.535*a + 2.181*aa)/(1.0+2.276*a+ 2.577*aa);
}


float D_Phong(vec3 N, vec3 H, float alpha)
{
    return (alpha+2.0)/ (2*pi) * pow(dot(N,H), alpha);
}

float G_Phong(vec3 V, vec3 H, float a)
{
    float vTan = approx_tan(V,H);
    if(vTan == 0.0) return 1.0;
    float alpha = sqrt(a/2+1.0) / vTan;
    return G_Phong_Beckman_impl(alpha);
}

vec3 PhongBRDF(vec3 L ,vec3 V , vec3 H , vec3 N , float alpha , vec3 Kd , vec3 Ks)
{
    float LH = dot(L,H);
    
    float D = D_Phong(N,H,alpha); // Ditribution
    vec3 F = Ks + (1.0-Ks) * pow(1-LH,5); // Fresnel approximation
    float G = G_Phong(V,H,alpha)* G_Phong(L,H,alpha); // Self-occluding self-shadowing
    
    return Kd/pi + D*F/4.0 * G;
}

float D_Beckham(vec3 N, vec3 H, float alpha)
{
    float alphaB = sqrt(2.0/ (alpha+2));
    float aBaB = alphaB*alphaB;
    float vTan = approx_tan(H,N);
    float NdotH = dot(N,H);

    return 1.0/ (pi*aBaB * NdotH*NdotH) * exp(-vTan*vTan/aBaB);
}

float G_Beckman(vec3 V, vec3 H, float a){
    float vTan = approx_tan(V,H);
    if(vTan == 0.0) return 1.0;
    float alpha = 1.0/(a*vTan);
    return G_Phong_Beckman_impl(alpha);
}

vec3 BeckhamBRDF(vec3 L ,vec3 V , vec3 H , vec3 N , float alpha , vec3 Kd , vec3 Ks)
{
    float LH = dot(L,H);
    
    float D = D_Beckham(N,H,alpha);
    vec3 F = Ks + (1.0-Ks) * pow(1-LH,5);
    float G = G_Beckman(L,H,alpha) * G_Beckman(V,H,alpha);
    
    return Kd/pi + D*F/4.0 * G;
}

float D_GGX(vec3 N, vec3 H , float alpha)
{
    //float alphaG = sqrt(2.0/ (alpha+2));
    float alphaG = alpha;
    float a2 = pow(alphaG,2);
    //float invAG = pow(alphaG-1.0,2);
    float NdotH = dot(N,H);
    float NdotH_sqr = NdotH * NdotH;

    float internalCacl = NdotH_sqr * (a2 - 1.0) + 1.0;

    //if (internalCacl == -1.0)
    //{
    //    return 1.0;
    //}

    float denom = (pi * pow( internalCacl + 1.0 , 2));
    float result = a2 / (denom+0.0001);
    
    return result;
}

float G_GGX(vec3 V, vec3 M , float a)
{
    //float alphaG = sqrt(2.0/ (a+2));
    float alphaG = a;

    float vTan = approx_tan(V,M);

    return 2.0/ (1.0+ sqrt(1+alphaG*alphaG*vTan*vTan));
}

vec3 GGXBRDF(vec3 L ,vec3 V , vec3 H , vec3 N , float alpha , vec3 Kd , vec3 Ks)
{
    float LH = dot(L,H);
  
    alpha = clamp(alpha, 0.001, 0.999);
    LH = clamp(LH, 0.001, 0.999);
    
    float D = D_GGX(N,H,alpha);
    
    vec3 comp1 = Ks + (vec3(1.0) - Ks);
    float vall = 1.0 - LH;
    float comp2 = pow(1.0 - LH, 5.0);
    vec3 F = Ks + (1.0 - Ks) * pow(1 - LH, 5);
    float G = G_GGX(L,H,alpha) * G_GGX(V,H,alpha);
    
    vec3 result = Kd / pi + D * F / 4.0 * G;   
   
    return result;
}



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
        sampledDepth = texture(sampler2D(samplerShadows,basicSampler), uvs).r;
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

float AttenuationFactor(float radius, float dist)
{
    float distsqr = dist * dist;
    float rsqr = radius * radius;
    float drsqr = distsqr + rsqr;
    return 2.0 / (drsqr + dist * sqrt(drsqr));
}

float getSquareFalloffAttenuation(vec3 posToLight, float lightInvRadius)
{
    float distanceSquare = dot(posToLight, posToLight);
    float factor = distanceSquare * lightInvRadius * lightInvRadius;
    float smoothFactor = max(1.0 - factor * factor, 0.0);
    return (smoothFactor * smoothFactor) / max(distanceSquare, 1e-4);
}

float UnrealFalloff(float dist, float radius)
{
    float num = clamp(1.0 - pow(dist / radius, 4.0), 0.0, 1.0);
    num = num * num;
    float denom = dist * dist + 1;
    return num / denom;
}

float Sascha_D_GGX(float dotNH, float roughness)
{
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
    return (alpha2) / (pi * denom * denom);
}

float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    float GL = dotNL / (dotNL * (1.0 - k) + k);
    float GV = dotNV / (dotNV * (1.0 - k) + k);
    return GL * GV;
}

vec3 F_Schlick(float cosTheta, float metallic, vec3 albedo)
{
    vec3 F0 = mix(albedo, vec3(0.04), metallic); // * material.specular
    vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    return F;
}



vec3 SaschaBRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness, vec3 lightColor, vec3 albedo)
{
    // Precalculate vectors and dot products	
    vec3 H = normalize(V + L);
    float dotNV = clamp(dot(N, V), 0.0, 1.0);
    float dotNL = clamp(dot(N, L), 0.0, 1.0);
    float dotLH = clamp(dot(L, H), 0.0, 1.0);
    float dotNH = clamp(dot(N, H), 0.0, 1.0);

	// Light color fixed

    vec3 color = vec3(0.0);

    //if (dotNL > 0.0)
    {
		// D = Normal distribution (Distribution of the microfacets)
        float D = Sascha_D_GGX(dotNH, roughness);
		// G = Geometric shadowing term (Microfacets shadowing)
        float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
		// F = Fresnel factor (Reflectance depending on angle of incidence)
        vec3 F = F_Schlick(dotNV, metallic, albedo);

        vec3 spec = D * F * G / (4.0 * dotNL * dotNV + 0.001);

        color += spec * dotNL * lightColor;
    }

    return color;
}

// Shlick's approximation of Fresnel
// https://en.wikipedia.org/wiki/Schlick%27s_approximation
vec3 Fresnel_Shlick(in vec3 f0, in vec3 f90, in float x)
{
    return f0 + (f90 - f0) * pow(1.f - x, 5);
}

// Burley B. "Physically Based Shading at Disney"
// SIGGRAPH 2012 Course: Practical Physically Based Shading in Film and Game Production, 2012.
float DiffuseBurley(in float NdotL, in float NdotV, in float LdotH, in float roughness)
{
    float fd90 = 0.5f + 2.f * roughness * LdotH * LdotH;
    return Fresnel_Shlick(vec3(1, 1, 1), vec3(fd90, fd90, fd90), NdotL).x * Fresnel_Shlick(vec3(1, 1, 1), vec3(fd90, fd90, fd90), NdotV).x;
}

// GGX specular D (normal distribution)
// https://www.cs.cornell.edu/~srm/publications/EGSR07-btdf.pdf
float Specular_D_GGX(in float alpha, in float NdotH)
{
    const float alpha2 = alpha * alpha;
    const float lower = (NdotH * NdotH * (alpha2 - 1)) + 1;
    return alpha2 / max(1e-5, pi * lower * lower);
}

// Schlick-Smith specular G (visibility) with Hable's LdotH optimization
// http://www.cs.virginia.edu/~jdl/bib/appearance/analytic%20models/schlick94b.pdf
// http://graphicrants.blogspot.se/2013/08/specular-brdf-reference.html
float G_Shlick_Smith_Hable(float alpha, float LdotH)
{
    return 1.0 / (mix(LdotH * LdotH, 1.0, alpha * alpha * 0.25f) + 0.001);
}

// A microfacet based BRDF.
//
// alpha:           This is roughness * roughness as in the "Disney" PBR model by Burley et al.
//
// specularColor:   The F0 reflectance value - 0.04 for non-metals, or RGB for metals. This follows model 
//                  used by Unreal Engine 4.
//
// NdotV, NdotL, LdotH, NdotH: vector relationships between,
//      N - surface normal
//      V - eye normal
//      L - light normal
//      H - half vector between L & V.
vec3 SpecularBRDF(in float alpha, in vec3 specularColor, in float NdotV, in float NdotL, in float LdotH, in float NdotH)
{
    // Specular D (microfacet normal distribution) component
    float specular_D = Specular_D_GGX(alpha, NdotH);

    // Specular Fresnel
    vec3 specular_F = Fresnel_Shlick(specularColor, vec3(1,1,1), LdotH);

    // Specular G (visibility) component
    float specular_G = G_Shlick_Smith_Hable(alpha, LdotH);

    return specular_D * specular_F * specular_G;
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

vec3 EvalLight(in LocalLightInstance lightInfo, in vec3 fragPos, in vec3 normal, float roughness, in vec3 albedo, float metalness)
{
    vec3 N = normal;    
    
    // Viewer to fragment
    vec3 V = normalize(uboFrameContext.cameraPosition.xyz - fragPos);
    
    // Vector to light
    vec3 L = lightInfo.position.xyz - fragPos;

	// Distance from light to fragment position
    float dist = length(L);
	
	// Light to fragment
    L = normalize(L);
                    
    vec3 H = normalize(L + V);
    float NdotL = max(0.0, dot(N, L));
    
    vec4 lightColInten = lightInfo.color;
    vec3 lCol = lightColInten.rgb * lightColInten.w;
    
    vec3 specular = vec3(0.0);
    metalness = clamp(metalness, 0.04, 0.95f);
    roughness = clamp(roughness, 0.04, 0.95f);
    specular += SaschaBRDF(L, V, N, metalness, roughness, lCol, albedo.rgb);
    
    float r1 = lightInfo.radius.x;
    float atten = UnrealFalloff(dist, lightInfo.radius.x);
    atten = 1.0;
    vec3 result = vec3(0.0f, 0.0f, 0.0f);
    result += specular * atten;
   
    if(false)
	{        
		//distribute the light across the area
       
    	// Attenuation
        atten = getSquareFalloffAttenuation(L, 1.0 / lightInfo.radius.x);
        
		// Specular part
        float alpha = roughness;
        vec3 Kd = albedo;
        vec3 Ks = vec3(metalness);
        vec3 diff = GGXBRDF(L, V, H, N, alpha, Kd, Ks) * NdotL * atten * lCol;

        vec3 R = -reflect(L, N);
        float RdotV = max(0.0, dot(R, V));
        vec3 spec = lCol * metalness
		* pow(RdotV, max(PC.specularModifier, 1.0))
		* atten;
        
        result = diff + spec;
    }
    
    return vec3(NdotL);
    return result;
}

vec3 EvalDirectionalLight(in vec4 lightCol
                        , in vec3 lightDir
                        , in vec3 fragPos
                        , in vec3 normal
                        , float roughness
                        , in vec3 albedo
                        , float metalness)
{
    vec3 N = normal;    
    // Viewer to fragment
    vec3 V = normalize(uboFrameContext.cameraPosition.xyz - fragPos);    
    // Vector to light
    vec3 L = normalize(lightDir);
    // Half vector
    vec3 H = normalize(L + V);
    
    vec4 lightColInten = lightCol;
    vec3 lCol = lightColInten.rgb * lightColInten.w;
    
    // vec3 specular = vec3(0.0);
    metalness = metalness;
    roughness = roughness;
    //specular += SaschaBRDF(L, V, N, metalness, roughness, lCol, albedo.rgb);
    //
    // vec3 result = vec3(0.0f, 0.0f, 0.0f);
    //result += specular;
  
    
    vec3 baseDiffusePBR = mix(albedo.rgb, vec3(0, 0, 0), metalness);

    // Specular coefficiant - fixed reflectance value for non-metals
    const float kSpecularCoefficient = 0.04f;
    vec3 baseSpecular = mix(vec3(kSpecularCoefficient, kSpecularCoefficient, kSpecularCoefficient), baseDiffusePBR, metalness); //* occlusion;
    
    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    // Burley roughness bias
    float alpha = roughness * roughness;

    // products
    float NdotL = clamp(dot(N, L), 0.0, 1.0);
    float LdotH = clamp(dot(L, H), 0.0, 1.0);
    float NdotH = clamp(dot(N, H), 0.0, 1.0);

    // Diffuse & specular factors
    float diffuseTerm = DiffuseBurley(NdotL, NdotV, LdotH, roughness);
    vec3 specularTerm = SpecularBRDF(alpha, baseSpecular, NdotV, NdotL, LdotH, NdotH);
    
    
    vec3 finalColor = NdotL * lCol * ((baseDiffusePBR * diffuseTerm) + specularTerm);
    
    //return vec3(NdotL);
    return finalColor;
}