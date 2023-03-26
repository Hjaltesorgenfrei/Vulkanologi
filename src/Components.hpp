#pragma once

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <memory>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "curves/Curves.hpp"

struct Transform {
    glm::mat4 modelMatrix = glm::mat4(1.0f);
};

struct RigidBody {
    btRigidBody* body;
};

struct Sensor {
    btGhostObject* ghost;
};

struct ControlPointPtr {
    const std::shared_ptr<ControlPoint> controlPoint; 
    // If this wrapper goes out of scope we don't crash but the points will only be accessible through the owning curve.

    ControlPointPtr() : controlPoint(std::make_shared<ControlPoint>()) {}
};

struct KeyboardInput {
    bool keys[GLFW_KEY_LAST] = {false};
    glm::vec2 mousePosition;
    glm::vec2 mouseDelta;
    bool mouseLeftPressed;
    bool mouseLeftReleased;
    bool mouseRightPressed;
    bool mouseRightReleased;
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
    btRaycastVehicle* vehicle;
    float steering = 0.0f;
    float acceleration = 0.0f;
    float brake = 0.0f;
    float maxSteering = 0.5f;
    float maxAcceleration = 2000.0f;
    float maxBrake = 100.0f;
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

struct Player {
    int id;
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    int lives = 3;
};

struct SpawnPoint {
    glm::vec3 position;
    glm::vec3 forward;
};

// TAGS

struct ShowNormalsTag {
    bool enabled = false;
};

struct MarkForDeletionTag { };

struct SelectedTag { };

#endif