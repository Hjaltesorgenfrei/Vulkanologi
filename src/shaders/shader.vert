#version 450

layout(set = 0, binding = 0) uniform UniformBufferObject {
	mat4 view;
	mat4 proj;
	mat4 projView;
} ubo;

layout (push_constant) uniform constants {
	mat4 modelMatrix;
} pushConstants;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragTexCoord;

void main() {
	gl_Position = ubo.projView * pushConstants.modelMatrix * vec4(inPosition, 1);
	fragColor = inColor;
	fragTexCoord = inTexCoord;
}
