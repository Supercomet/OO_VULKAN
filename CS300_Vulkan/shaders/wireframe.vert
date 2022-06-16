#version 450 // use GLSL 4.5

layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 tex;

// vulkan passes a whole Uniform Buffer Object.
layout(set = 1,binding = 0) uniform UboViewProjection{
	mat4 projection;
	mat4 view;
}uboViewProjection;

//
//layout(set = 0,binding = 1) uniform UboModel{
//	mat4 model;
//}uboModel;

layout(push_constant)uniform data{
		mat4 viewproj;
		mat4 xform;
}push;

layout(location = 0) out vec3 fragCol;
layout(location = 1) out vec2 fragTex;

void main(){

vec4 trans = push.xform * vec4(pos, 1.0);
gl_Position = push.viewproj *  trans;

fragCol = col;
fragTex = tex;
}