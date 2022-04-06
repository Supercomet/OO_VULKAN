#version 450 // use GLSL 4.5

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 col;
layout(location = 2) in vec2 tex;

// Instanced attributes
layout (location = 4) in vec3 instancePos;
layout (location = 5) in vec3 instanceRot;
layout (location = 6) in float instanceScale;
layout (location = 7) in int instanceTexIndex;

// vulkan passes a whole Uniform Buffer Object.
layout(set = 0,binding = 0) uniform UboViewProjection{
	mat4 projection;
	mat4 view;
}uboViewProjection;

//
//layout(set = 0,binding = 1) uniform UboModel{
//	mat4 model;
//}uboModel;


layout(location = 0) out vec3 fragCol;
layout(location = 1) out vec2 fragTex;
layout(location = 2) flat out int outTexIndex;

void main(){

	mat4 mx, my, mz;
	
	// rotate around x
	float s = sin(instanceRot.x);
	float c = cos(instanceRot.x);

	mx[0] = vec4(c, s, 0.0, 0.0);
	mx[1] = vec4(-s, c, 0.0, 0.0);
	mx[2] = vec4(0.0, 0.0, 1.0, 0.0);
	mx[3] = vec4(0.0, 0.0, 0.0, 1.0);	
	
	// rotate around y
	s = sin(instanceRot.y);
	c = cos(instanceRot.y);

	my[0] = vec4(c, 0.0, s, 0.0);
	my[1] = vec4(0.0, 1.0, 0.0, 0.0);
	my[2] = vec4(-s, 0.0, c, 0.0);
	my[3] = vec4(0.0, 0.0, 0.0, 1.0);	
	
	// rot around z
	s = sin(instanceRot.z);
	c = cos(instanceRot.z);	
	
	mz[0] = vec4(1.0, 0.0, 0.0, 0.0);
	mz[1] = vec4(0.0, c, s, 0.0);
	mz[2] = vec4(0.0, -s, c, 0.0);
	mz[3] = vec4(0.0, 0.0, 0.0, 1.0);	
	
	mat4 rotMat = mz * my * mx;

	vec4 pos = rotMat * vec4((inPos.xyz * instanceScale) + instancePos, 1.0);

	gl_Position = uboViewProjection.projection * uboViewProjection.view * pos;
	outTexIndex = instanceTexIndex;
	fragCol = col;
	fragTex = tex;
}