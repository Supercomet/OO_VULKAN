#version 450

#extension GL_EXT_nonuniform_qualifier : require

//layout (set = 0, binding = 1) uniform sampler2D textures[];
layout(set = 1, binding= 0) uniform sampler2D textureSampler;

layout(location = 0) in vec3 fragCol;
layout(location = 1) in vec2 fragTex;
layout (location = 2) flat in int inTextureIndex;


layout(location = 0) out vec4 outColour; //final output colour (Must also have location!)

void main(){
//vec4 color = texture(textures[nonuniformEXT(inTextureIndex)], fragTex.xy);
outColour = texture(textureSampler,fragTex);
//outColour = vec4(1.0,0.0,0.0,1.0);
}