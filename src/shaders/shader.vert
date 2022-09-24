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
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;
layout(location = 4) in uint inMaterialIndex;


layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out uint materialIndex;

const vec3 DIRECTION_TO_LIGHT = normalize(vec3(1.0, 3.0, -1.0));
const float AMBIENT = 0.02;

void main() {
	// Diffuse shading does not work correctly if model is scaled non Uniformly
	// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
	// TODO: Fix this if it becomes necessary
	// Can be done by computing the value on the CPU and sending it as a Push Constant
	vec3 normalWorldSpace = normalize((pushConstants.modelMatrix * vec4(inNormal, 0.0)).xyz);  
	float lightIntensity = AMBIENT + max(dot(normalWorldSpace, DIRECTION_TO_LIGHT), 0);

	gl_Position = ubo.projView * pushConstants.modelMatrix * vec4(inPosition, 1);
	fragColor = inColor * lightIntensity;
	fragTexCoord = inTexCoord;
	materialIndex = inMaterialIndex;
}
