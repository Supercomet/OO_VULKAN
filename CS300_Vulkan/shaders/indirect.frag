#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (set = 1, binding = 0) uniform sampler2D textureDesArr[];
//layout(set = 1, binding= 0) uniform sampler2D textureSampler;

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inUV;
layout (location = 2) flat in int inTextureIndex;

layout(location = 3)in vec3 inNormal;
layout(location = 4)in vec3 inLightVec;
layout(location = 5)in vec3 inViewVec;

layout(location = 0) out vec4 outColour; //final output colour (Must also have location!)

void main(){
vec4 color = texture( textureDesArr[nonuniformEXT(inTextureIndex)], inUV.xy);

if (color.a < 0.5)
	{
		discard;
	}

vec3 N = normalize(inNormal);
vec3 L = normalize(inLightVec);
vec3 ambient = vec3(0.65);
vec3 diffuse = max(dot(N, L), 0.0) * inColor;
outColour = vec4((ambient + diffuse) * color.rgb, 1.0);

//outColour =  color;
}