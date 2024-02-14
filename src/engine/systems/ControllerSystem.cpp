#include "ControllerSystem.hpp"

#include <GLFW/glfw3.h>

#include "../Components.hpp"

const std::string_view ControllerSystem::name() {
	return "ControllerSystem";
}

const std::unordered_set<std::type_index> ControllerSystem::reads() {
	static auto set = std::unordered_set<std::type_index>{};
	return set;
}

const std::unordered_set<std::type_index> ControllerSystem::writes() {
	static auto set = std::unordered_set<std::type_index>{typeid(GamepadInput)};
	return set;
}

void ControllerSystem::update(entt::registry &registry, float delta) const {
	registry.view<GamepadInput>().each([this, &registry, delta](entt::entity ent, GamepadInput input) {
		// Get Joystick Input
		auto joystickId = input.joystickId;
		if (glfwJoystickPresent(joystickId) != GLFW_TRUE) {
			input.timeSinceLastSeen += delta;

			// If we haven't seen the controller for 10 seconds, assume it's disconnected
			if (input.timeSinceLastSeen > 10.f) {
				input.connected = false;
			}
			return;
		}
		int axesCount;
		auto axes = glfwGetJoystickAxes(joystickId, &axesCount);
		int buttonsCount;
		auto buttons = glfwGetJoystickButtons(joystickId, &buttonsCount);
		if (axesCount < GLFW_GAMEPAD_AXIS_LAST || buttonsCount < GLFW_GAMEPAD_BUTTON_LAST) {
			return;  // Not a valid controller
		}

		// left stick
		float x = axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		float y = axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		float deadzone = 0.2f;
		if (x < -deadzone || x > deadzone) {
			input.leftStick.x = 0.f;
		}
		if (y < -deadzone || y > deadzone) {
			input.leftStick.y = 0.f;
		}
		input.leftStick.x = x;
		input.leftStick.y = y;

		// right stick
		x = axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
		y = axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
		if (x < -deadzone || x > deadzone) {
			input.rightStick.x = 0.f;
		}
		if (y < -deadzone || y > deadzone) {
			input.rightStick.y = 0.f;
		}
		input.rightStick.x = x;
		input.rightStick.y = y;

		// left trigger
		float leftTrigger = axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
		// remap from [-1, 1] to [0, 1]
		leftTrigger = (leftTrigger + 1.f) / 2.f;
		if (leftTrigger < deadzone) {
			input.leftTrigger = 0.f;
		}
		input.leftTrigger = leftTrigger;

		// right trigger
		float rightTrigger = axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
		// remap from [-1, 1] to [0, 1]
		rightTrigger = (rightTrigger + 1.f) / 2.f;
		if (rightTrigger < deadzone) {
			input.rightTrigger = 0.f;
		}
		input.rightTrigger = rightTrigger;

		// buttons
		input.a = buttons[GLFW_GAMEPAD_BUTTON_A] == GLFW_PRESS;
		input.b = buttons[GLFW_GAMEPAD_BUTTON_B] == GLFW_PRESS;
		input.x = buttons[GLFW_GAMEPAD_BUTTON_X] == GLFW_PRESS;
		input.y = buttons[GLFW_GAMEPAD_BUTTON_Y] == GLFW_PRESS;
		input.leftBumper = buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] == GLFW_PRESS;
		input.rightBumper = buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] == GLFW_PRESS;
		input.up = buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS;
		input.down = buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS;
		input.left = buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] == GLFW_PRESS;
		input.right = buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] == GLFW_PRESS;
		input.leftStickButton = buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB] == GLFW_PRESS;
		input.rightStickButton = buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB] == GLFW_PRESS;
		input.start = buttons[GLFW_GAMEPAD_BUTTON_START] == GLFW_PRESS;
		input.back = buttons[GLFW_GAMEPAD_BUTTON_BACK] == GLFW_PRESS;
		input.guide = buttons[GLFW_GAMEPAD_BUTTON_GUIDE] == GLFW_PRESS;
	});
}