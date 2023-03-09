#version 450
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_GOOGLE_include_directive : enable

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
	GlobalUbo ubo;
};

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(fragColor, 1.0);
}