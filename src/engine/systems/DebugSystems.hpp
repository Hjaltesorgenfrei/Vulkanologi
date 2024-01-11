#pragma once
#include "../Components.hpp"
#include "../DependentSystem.hpp"

struct DebugCameraKeyboardSystem
	: System<DebugCameraKeyboardSystem, Reads<KeyboardInput, MouseInput>, Writes<Camera>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, KeyboardInput const &keyboardInput,
				MouseInput const &mouseInput, Camera &camera) const;
};

struct DebugCameraJoystickSystem : System<DebugCameraJoystickSystem, Reads<GamepadInput>, Writes<Camera>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, GamepadInput const &input,
				Camera &camera) const;
};