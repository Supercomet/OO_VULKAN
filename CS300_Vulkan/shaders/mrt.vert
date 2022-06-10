#version 450 // use GLSL 4.5

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec2 inUV;

// vulkan passes a whole Uniform Buffer Object.
layout(set = 0,binding = 0) uniform UboViewProjection{
	mat4 projection;
	mat4 view;
	vec4 camPos;
}uboViewProjection;

layout(push_constant)uniform PushLight{
		mat4 instanceMatrix;
		vec3 pos;
}pushLight;

layout(location = 0) out vec4 outPos;
layout(location = 1) out vec2 outUV;

//layout(location = 1) out flat  struct{
//  ivec4 maps;
//  //int albedo;	
// //int normal;	
// //int occlusion;
// //int roughness;
//}outTexIndex;


layout(location = 7) out struct 
{
mat3 btn;
vec3 vertCol;
vec3 localVertexPos;
vec3 localLightPos;
vec3 localEyePos;
}outLightData;


void main(){

	// inefficient
	mat4 inverseMat = inverse(pushLight.instanceMatrix);
	
	outLightData.localEyePos = vec3(inverseMat* vec4(vec3(uboViewProjection.camPos),1.0));
	
	outLightData.localLightPos = vec3(inverseMat * vec4(pushLight.pos, 1.0));

	vec3 binormal = normalize(cross(inTangent, inNormal));
	outLightData.btn = mat3(inTangent, binormal, inNormal);


	//outViewVec = -vec3(uboViewProjection.view[3]);	
	outLightData.localVertexPos = inPos;

	outPos = pushLight.instanceMatrix * vec4(inPos,1.0);
	gl_Position = uboViewProjection.projection * uboViewProjection.view * outPos;
	//outTexIndex.maps.x = instanceTexIndex;
	//outTexIndex.maps.y = instanceNormalTexIndex;
	//outTexIndex.maps.z = instanceOcclusionTexIndex;
	//outTexIndex.maps.w = instanceRoughnessTexIndex;
	outUV = inUV;
}