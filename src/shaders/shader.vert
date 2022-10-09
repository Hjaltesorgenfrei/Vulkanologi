#version 450

#include "utils/global_ubo.glsl"
layout(set = 0, binding = 0) uniform UniformBufferObject {
	GlobalUbo ubo;
};

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
layout(location = 3) out vec3 fragPosWorld;
layout(location = 4) out vec3 fragNormalWorld;

void main() {
	vec4 positionWorld = pushConstants.modelMatrix * vec4(inPosition, 1.0);

	gl_Position = ubo.projView * positionWorld;
	fragColor = inColor;
	fragTexCoord = inTexCoord;
	materialIndex = inMaterialIndex;
	fragPosWorld = positionWorld.xyz;
	// Diffuse shading does not work correctly if model is scaled non Uniformly
	// https://paroj.github.io/gltut/Illumination/Tut09%20Normal%20Transformation.html
	// TODO: Fix this if it becomes necessary
	// Can be done by computing the value on the CPU and sending it as a Push Constant
	fragNormalWorld = normalize((pushConstants.modelMatrix * vec4(inNormal, 0.0)).xyz);
}
