#version 450
#extension GL_GOOGLE_include_directive : enable

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
	GlobalUbo ubo;
};

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 outUVW;

void main() 
{
	gl_Position = ubo.proj * ubo.view * vec4(inPosition, 1.0);
}