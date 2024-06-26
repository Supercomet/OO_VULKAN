#ifndef COMMON_HOST_DEVICE
#define COMMON_HOST_DEVICE


#define BIND_POINT_VERTEX_BUFFER_ID  0
#define BIND_POINT_INSTANCE_BUFFER_ID  2
#define BIND_POINT_GPU_SCENE_BUFFER_ID  3


#ifdef __cplusplus
#include "MathCommon.h"
// GLSL Type
using vec4 = glm::vec4;
using vec3 = glm::vec3;
using vec2 = glm::vec2;
using uvec4 = glm::uvec4;
using uvec3 = glm::uvec3;
using uvec2 = glm::uvec2;
using ivec4 = glm::ivec4;
using ivec3 = glm::ivec3;
using ivec2 = glm::ivec2;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

struct CustomIndirectCommand
{
    //VkDrawIndexedIndirectCommand;
    uint    indexCount;
    uint    instanceCount;
    uint    firstIndex;
    int     vertexOffset;
    uint    firstInstance;
    vec4    sphere;
};

struct LocalLightInstance
{
    // info.x > 0 cast shadow,
    // info.y grid index
    // info.z 0 dont render
    // info.w 1 point, 2 area
    ivec4 info;// TODO: does this take up too much space?
    vec4 position;
    vec4 color;
    vec4 radius;
    mat4 projection;
    mat4 view[6];
};

struct OmniLightInstance
{
    ivec4 info; 
    vec4 position; // XYZ
    vec4 color; // RGB Intensity
    vec4 radius; // Inner rad outer rad etc..
    mat4 projection;
    mat4 view[6];
};

struct SpotLightInstance
{
    ivec4 info;
    vec4 position;
    vec4 color;
    vec4 radius; // x inner, y outer
    mat4 projection;
    mat4 view;
};

struct AreaLightInstance
{
    ivec4 info;
    vec4 position;
    vec4 color;
    vec4 radius; // xy z=twoSided
    mat4 projection;
    mat4 view; // for shadowing
    vec4 points[4];  
};

struct LightPC
{
    uint numLights;
    uint useSSAO;
    vec2 shadowMapGridDim;
    float ambient;
    float maxBias;
    float mulBias;
    float specularModifier;
    vec4 directionalLight;
    vec4 lightColorInten;
    vec2 resolution;
};

struct SSAOPC
{
    vec2 screenDim;
    vec2 sampleDim; // screenDim_sampleDim
    float radius;
    float bias;
    float intensity;
    uint numSamples;
};

struct BloomPC
{
    vec4 threshold;
};

struct ColourCorrectPC
{
    vec4 shadowCol;
    vec4 midCol;
    vec4 highCol;
    vec2 threshold;
    float exposure;
};

struct VignettePC
{
    vec4 colour;
    vec4 vignetteValues;

};

struct CullingPC
{
    vec4 top;
    vec4 bottom;
    vec4 right;
    vec4 left;
    vec4 pFar;
    vec4 pNear;
    uint numItems;
};

struct GPUTransform
{
	vec4 row0;
	vec4 row1;
	vec4 row2;
    vec4 invRow0;
    vec4 invRow1;
    vec4 invRow2;
    vec4 prevRow0;
    vec4 prevRow1;
    vec4 prevRow2;
	vec4 colour; // temp
};

const uint MAX_BONE_NUM = 4;
struct BoneWeight
{
    uint boneIdx[MAX_BONE_NUM];
    float boneWeights[MAX_BONE_NUM];
};

// struct represents perobject information in gpu
struct GPUObjectInformation
{
    uint boneStartIdx;
    int entityID;
    uint materialIdx;
    uint boneWeightsOffset;
    vec4 emissiveColour;
};

struct HistoStruct{
    uint histoBin[256];
    float cdf[256];
};

struct LuminenceData {
    float LumOut;
    float AverageLogLum;
};

struct LuminencePC {
    float minLogLum;
    float deltaLogLum;
    float timeCoeff;
    float pixelsSampled;
};

#endif //! COMMON_HOST_DEVICE
