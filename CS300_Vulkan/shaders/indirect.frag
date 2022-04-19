#version 450

#extension GL_EXT_nonuniform_qualifier : require

layout (set = 1, binding = 0) uniform sampler2D textureDesArr[];
//layout(set = 1, binding= 0) uniform sampler2D textureSampler;

layout(location = 0) in vec2 inUV;

layout(location = 1) in flat struct{
 ivec4 maps;
 //int albedo;	
 //int normal;	
 //int occlusion;
 //int roughness;
}inTexIndex;

layout(location = 3)in vec3 inNormal;

layout(location = 4)in vec3 inViewVec;

layout(location = 7) in struct {
mat3 btn;
vec3 vertCol;
vec3 localVertexPos;
vec3 localLightPos;
vec3 localEyePos;
}inLightData;


layout(location = 0) out vec4 outColour; //final output colour (Must also have location!)

void main(){
vec4 color = texture( textureDesArr[nonuniformEXT(inTexIndex.maps.x)], inUV.xy);
vec3 normal = texture( textureDesArr[nonuniformEXT(inTexIndex.maps.y)], inUV.xy).rgb;

color.rgb = pow(color.rgb,vec3(2.2));

// Decode to -1 to 1 for each read element
normal.xy =  normal.gr * 2.0 - 1.0;

// Derive the final element since we only have (x,y)
// This also forces the Normal map to be normalize
normal.z =  sqrt(1.0 - dot(normal.xy, normal.xy));
normal = inLightData.btn * normal;

//if (color.a < 0.5)
//	{
//		discard;
//	}
//
// Note that the real light direction is the negative of this, but the negative is removed to speed up the equations
vec3 LightDirection = normalize( inLightData.localLightPos.xyz - inLightData.localVertexPos);

// Compute the diffuse intensity
float DiffuseI  = max( 0, dot( normal, LightDirection ));

// Note This is the true Eye to Texel direction 
vec3 EyeDirection = normalize( inLightData.localVertexPos - inLightData.localEyePos.xyz );

// Determine the power for the specular based on how rough something is
const float Shininess = mix( 1, 100, 1 - texture( textureDesArr[nonuniformEXT(inTexIndex.maps.w)], inUV).r );

float SpecularI2  = pow( max( 0, dot(normal, normalize( LightDirection - EyeDirection ))), Shininess );

vec3 ambientLightCol = vec3(0.3,0.3,0.3);
outColour.rgb  = ambientLightCol * color.rgb * texture(textureDesArr[nonuniformEXT(inTexIndex.maps.z)], inUV).rgb;

// Add the contribution of this light
vec3 lightCol = vec3(1.0,0.3,0.3);
outColour.rgb += lightCol * (SpecularI2* 1.0 *DiffuseI.rrr * color.rgb );

// Convert to gamma
const float Gamma = 2.20;
outColour.a   = color.a;
outColour.rgb = pow( outColour.rgb, vec3(1.0f/Gamma) );

//outColour = vec4(color.rgb, 1.0);

}