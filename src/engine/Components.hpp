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

// Struct to store speed, direction and position of a car for the last frame.
struct CarStateLastUpdate {
	float speed;
	glm::vec3 direction;
	glm::vec3 position;
};

struct CarSettings {
	bool settingsChanged = false;  // If this value changes the car should be recreated

	float sMaxEngineTorque = 500.0f;
	float sClutchStrength = 10.0f;
	bool sLimitedSlipDifferentials = true;
	float halfVehicleLength = 2.0f;
	float halfVehicleWidth = 0.9f;
	float halfVehicleHeight = 0.2f;

	float wheelRadius = 0.3f;
	float wheelWidth = 0.1f;

	float maxRollAngle = 60.0f;
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