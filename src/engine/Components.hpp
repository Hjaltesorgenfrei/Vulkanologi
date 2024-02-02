#pragma once

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <memory>

#include "BehCamera.hpp"
#include "Util.hpp"
#include "curves/Curves.hpp"

struct Transform {
	glm::mat4 modelMatrix = glm::mat4(1.0f);
};

struct ControlPointPtr {
	const std::shared_ptr<ControlPoint> controlPoint;
	// If this wrapper goes out of scope we don't crash but the points will only be accessible through the owning curve.

	ControlPointPtr() : controlPoint(std::make_shared<ControlPoint>()) {}
};

struct KeyboardInput {
	bool keys[GLFW_KEY_LAST] = {false};
};

struct MouseInput {
	bool buttons[GLFW_MOUSE_BUTTON_LAST] = {false};
	glm::vec2 mousePosition;
	glm::vec2 mouseDelta;
	bool cursorDisabled = false;
};

struct GamepadInput {
	int joystickId;
	float timeSinceLastSeen = 0.f;
	bool connected = true;

	// Buttons
	bool a;
	bool b;
	bool x;
	bool y;
	bool leftBumper;
	bool rightBumper;
	bool up;
	bool down;
	bool left;
	bool right;
	bool leftStickButton;
	bool rightStickButton;
	bool start;
	bool back;
	bool guide;

	// Axes
	glm::vec2 leftStick;
	glm::vec2 rightStick;
	float leftTrigger;
	float rightTrigger;
};

struct Car {
	float steering = 0.0f;
	float acceleration = 0.0f;
	float brake = 0.0f;
	float maxSteering = 0.5f;
	float maxBrake = 200.0f;
};

struct CarControl {
	float desiredSteering = 0.0f;
	float desiredAcceleration = 0.0f;
	float desiredBrake = 0.0f;
};

// TODO: Organize the values better. And add the rest of the values.
// TODO: Add a short description of each value and its unit
struct CarSettings {
	float maxEngineTorque = 50000.0f;
	float maxSteeringAngle = 30.0f;

	bool limitedSlipDifferentials = true;
	bool fourWheelDrive = false;
	bool antiRollbar = true;

	float clutchStrength = 10.0f;
	float halfVehicleLength = 2.0f;
	float halfVehicleWidth = 0.9f;
	float halfVehicleHeight = 0.2f;

	float wheelRadius = 0.3f;
	float wheelWidth = 0.1f;

	float maxRollAngle = 60.0f;

	// TODO: Figure out what this name should be, is it steering?
	struct Suspension {
		float casterAngle = 0.0f;
		float kingPinAngle = 0.0f;
		float camber = 0.0f;
		float toe = 0.0f;

		// TODO: This values should maybe be a part of a suspension struct instead.
		float suspensionForwardAngle = 0.0f;
		float suspensionSidewaysAngle = 0.0f;
		float suspensionMinLength = 0.3f;
		float suspensionMaxLength = 0.5f;
		float suspensionFrequency = 1.5f;
		float suspensionDamping = 0.5f;
	} front, rear;
};

struct Player {
	int id;
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
	int lives = 3;
	bool isAlive = true;
};

struct SpawnPoint {
	glm::vec3 position;
	glm::vec3 forward;
};

struct Swiper {
	Axis axis;
	float speed = 0.0f;
};

struct Camera {
	BehCamera camera{};
};

struct Networked {
	uint64_t id;
};

// TAGS

struct ShowNormalsTag {
	bool enabled = false;
};

struct MarkForDeletionTag {};

struct SelectedTag {};

struct SensorTag {};

struct ActiveCameraTag {};

struct PlayerCube {};

#endif