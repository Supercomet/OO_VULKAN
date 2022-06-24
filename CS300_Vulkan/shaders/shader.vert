#version 450 // use GLSL 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 norm;
layout(location = 2) in vec3 inCol;
layout(location = 4) in vec2 tex;

// vulkan passes a whole Uniform Buffer Object.
layout(set = 1,binding = 0) uniform UboViewProjection{
	mat4 projection;
	mat4 view;
}uboViewProjection;

//
//layout(set = 0,binding = 1) uniform UboModel{
//	mat4 model;
//}uboModel;

layout(push_constant)uniform PushModel{
		mat4 model;
}pushModel;

layout(location = 0) out vec3 outCol;
layout(location = 1) out vec2 fragTex;

void main(){

gl_Position = uboViewProjection.projection * uboViewProjection.view *  vec4(pos, 1.0);

outCol = inCol;
fragTex = tex;
}