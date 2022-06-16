#ifndef COMMON_HOST_DEVICE
#define COMMON_HOST_DEVICE

#ifdef __cplusplus
#include <glm/glm.hpp>
// GLSL Type
using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using uvec4 = glm::uvec4;
using mat4 = glm::mat4;
using uint = unsigned int;
#endif

struct GPUTransform
{
	vec4 row0;
	vec4 row1;
	vec4 row2;
};

#endif //! COMMON_HOST_DEVICE
