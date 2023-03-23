#include "CarSystem.h"
#include "../Components.h"

void carSystemUpdate(entt::registry &registry, float delta)
{
    auto view = registry.view<Car, KeyboardInput>();

    for (auto entity : view) {
        auto& car = view.get<Car>(entity);
        auto& keyboardInput = view.get<KeyboardInput>(entity);

        if (keyboardInput.keys[GLFW_KEY_RIGHT]) {
            car.desiredSteering = -0.5f;
        }
        else if (keyboardInput.keys[GLFW_KEY_LEFT]) {
            car.desiredSteering = 0.5f;
        }
        else {
            car.desiredSteering = 0.0f;
        }

        if (keyboardInput.keys[GLFW_KEY_UP]) {
            car.desiredAcceleration = 1000.0f;
        }
        else if (keyboardInput.keys[GLFW_KEY_DOWN]) {
            car.desiredAcceleration = -1000.0f;
        }
        else {
            car.desiredAcceleration = 0.0f;
        }

        if (keyboardInput.keys[GLFW_KEY_SPACE]) {
            car.desiredBrake = 1000.0f;
        }
        else {
            car.desiredBrake = 0.0f;
        }
        
        auto currentSteering = car.vehicle->getSteeringValue(0);
        auto currentAcceleration = car.vehicle->getWheelInfo(2).m_engineForce;
        auto currentBrake = car.vehicle->getWheelInfo(2).m_brake;

        car.steering = glm::mix(currentSteering, car.desiredSteering, 0.01f * delta);
        car.acceleration = glm::mix(currentAcceleration, car.desiredAcceleration, 0.01f * delta);
        car.brake = glm::mix(currentBrake, car.desiredBrake, 0.01f * delta);

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