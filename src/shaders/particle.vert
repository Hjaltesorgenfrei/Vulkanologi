#version 450
#extension GL_GOOGLE_include_directive : enable

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
	GlobalUbo ubo;
};

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec3 fragColor;

void main() {
	gl_PointSize = 14.0;
	gl_Position = vec4(inPosition.xy, 0, 1);
	fragColor = inColor.rgb;
}
