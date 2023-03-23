#include <cmath>
#include "CarSystem.h"
#include "../Components.h"
#include "ControllerSystem.h"

void carSystemUpdate(entt::registry &registry, float delta)
{
    for (auto entity : registry.view<CarControl, KeyboardInput>()) {
        auto& carControl = registry.get<CarControl>(entity);
        auto& keyboardInput = registry.get<KeyboardInput>(entity);

        if (keyboardInput.keys[GLFW_KEY_RIGHT]) {
            carControl.desiredSteering = -0.5f;
        }
        else if (keyboardInput.keys[GLFW_KEY_LEFT]) {
            carControl.desiredSteering = 0.5f;
        }
        else {
            carControl.desiredSteering = 0.0f;
        }

        if (keyboardInput.keys[GLFW_KEY_UP]) {
            carControl.desiredAcceleration = 1000.0f;
        }
        else if (keyboardInput.keys[GLFW_KEY_DOWN]) {
            carControl.desiredAcceleration = -1000.0f;
        }
        else {
            carControl.desiredAcceleration = 0.0f;
        }

        if (keyboardInput.keys[GLFW_KEY_SPACE]) {
            carControl.desiredBrake = 1000.0f;
        }
        else {
            carControl.desiredBrake = 0.0f;
        }
    }

    for (auto entity : registry.view<Car, CarControl, ControllerInput>()) {
        auto& car = registry.get<Car>(entity);
        auto& carControl = registry.get<CarControl>(entity);
        auto& controllerInput = registry.get<ControllerInput>(entity);
        auto currentSpeed = car.vehicle->getCurrentSpeedKmHour();
        
        carControl.desiredSteering = -(controllerInput.leftStick.x * 0.5f);
        if (currentSpeed > 0.0f) {
            carControl.desiredAcceleration = controllerInput.rightTrigger * 1000.0f;
            carControl.desiredBrake = controllerInput.leftTrigger * 1000.0f;
        }
        else {
            carControl.desiredAcceleration = -controllerInput.leftTrigger * 1000.0f;
            carControl.desiredBrake = controllerInput.rightTrigger * 1000.0f;
        }
    }

    for (auto entity : registry.view<Car, CarControl>()) {
        auto& carControl = registry.get<CarControl>(entity);
        auto& car = registry.get<Car>(entity);
        auto currentSteering = car.vehicle->getSteeringValue(0);
        auto currentAcceleration = car.vehicle->getWheelInfo(2).m_engineForce;
        auto currentBrake = car.vehicle->getWheelInfo(2).m_brake;
        
        float desiredSteering = std::clamp(carControl.desiredSteering, -car.maxSteering, car.maxSteering);
        float desiredAcceleration = std::clamp(carControl.desiredAcceleration, -car.maxAcceleration, car.maxAcceleration);
        float desiredBrake = std::clamp(carControl.desiredBrake, 0.0f, car.maxBrake);

        car.steering = glm::mix(currentSteering, desiredSteering, 0.01f * delta);
        car.acceleration = glm::mix(currentAcceleration, desiredAcceleration, 0.01f * delta);
        car.brake = glm::mix(currentBrake, desiredBrake, 0.01f * delta);

        car.vehicle->applyEngineForce(car.acceleration, 2);
        car.vehicle->applyEngineForce(car.acceleration, 3);
        car.vehicle->setSteeringValue(car.steering, 0);
        car.vehicle->setSteeringValue(car.steering, 1);
        car.vehicle->setBrake(car.brake, 0);
        car.vehicle->setBrake(car.brake, 1);
        car.vehicle->setBrake(car.brake, 2);
        car.vehicle->setBrake(car.brake, 3);
    }
}