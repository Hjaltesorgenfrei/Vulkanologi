#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
	GlobalUbo ubo;
};

layout(set = 1, binding = 0) uniform samplerCube samplerCubeMaps[];

layout(location = 0) in vec3 inUVW;

layout(location = 0) out vec4 outFragColor;

void main() {
	outFragColor = texture(samplerCubeMaps[0], inUVW);
}