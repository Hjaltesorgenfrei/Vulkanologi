#pragma once

#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <memory>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include "curves/Curves.h"

struct Transform {
    glm::mat4 modelMatrix;
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
    
    void clear() {
        for (int i = 0; i < GLFW_KEY_LAST; i++) {
            keys[i] = false;
        }
        mouseLeftPressed = false;
        mouseLeftReleased = false;
        mouseRightPressed = false;
        mouseRightReleased = false;
        mousePosition = glm::vec2(0.0f);
        mouseDelta = glm::vec2(0.0f);
    }
};

struct ControllerInput {
    std::unordered_set<int> buttonsPressed;
    std::unordered_set<int> buttonsReleased;
    std::unordered_set<int> buttonsDown;
    glm::vec2 leftStick;
    glm::vec2 rightStick;
    float leftTrigger;
    float rightTrigger;
    
    void clear() {
        buttonsPressed.clear();
        buttonsReleased.clear();
        leftStick = glm::vec2(0.0f);
        rightStick = glm::vec2(0.0f);
        leftTrigger = 0.0f;
        rightTrigger = 0.0f;
    }
};

struct Car {
    btRaycastVehicle* vehicle;
    float steering = 0.0f;
    float desiredSteering = 0.0f;
    float acceleration = 0.0f;
    float desiredAcceleration = 0.0f;
    float brake = 0.0f;
    float desiredBrake = 0.0f;
};

#endif