#version 450
#extension GL_GOOGLE_include_directive : enable

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
	GlobalUbo ubo;
};

layout(push_constant) uniform constants {
	mat4 modelMatrix;
	vec4 color;
}
pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uint inMaterialIndex;

layout(location = 0) out vec3 outUVW;

void main() {
	outUVW = inPosition;
	gl_Position = ubo.proj * pushConstants.modelMatrix * vec4(inPosition.xyz, 1.0);
}