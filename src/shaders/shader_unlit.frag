#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4 projView;
	vec4 ambientLightColor; // w is intensity
	vec4 lightPosition; // w is ignored
	vec4 lightColor; // w is intensity
} ubo;

layout(set = 1, binding = 0) uniform sampler2D texSampler[];

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in flat uint materialIndex;
layout(location = 3) in vec3 fragPosWorld;
layout(location = 4) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4(fragColor, 1.0);
}