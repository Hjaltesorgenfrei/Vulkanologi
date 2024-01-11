#include "DebugSystems.hpp"

void DebugCameraKeyboardSystem::update(entt::registry& registry, float delta, entt::entity ent,
									   KeyboardInput const& input, Camera& camera) const {
	if (!registry.any_of<ActiveCameraTag>(ent)) {
		return;
	}

	float cameraSpeed = 0.005f * delta;
	if (input.keys[GLFW_KEY_LEFT_SHIFT]) {
		cameraSpeed *= 4;
	}
	if (input.keys[GLFW_KEY_W] == GLFW_PRESS) camera.camera.moveCameraForward(cameraSpeed);
	if (input.keys[GLFW_KEY_S] == GLFW_PRESS) camera.camera.moveCameraBackward(cameraSpeed);
	if (input.keys[GLFW_KEY_A] == GLFW_PRESS) camera.camera.moveCameraLeft(cameraSpeed);
	if (input.keys[GLFW_KEY_D] == GLFW_PRESS) camera.camera.moveCameraRight(cameraSpeed);
}

void DebugCameraJoystickSystem::update(entt::registry& registry, float delta, entt::entity ent,
									   GamepadInput const& input, Camera& camera) const {
	float cameraSpeed = 0.005f * delta;
}
