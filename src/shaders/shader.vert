#version 450

layout(binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4 projView;
} ubo;

layout (push_constant) uniform constants {
	vec4 model;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
	gl_Position = ubo.projView * vec4(inPosition + pushConstants.model.xyz, 1.0);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}
