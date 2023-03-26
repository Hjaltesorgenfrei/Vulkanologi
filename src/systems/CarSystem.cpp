#include <cmath>
#include "../Util.hpp"
#include "CarSystem.hpp"

void CarSystem::update(entt::registry &registry, float delta, entt::entity ent, CarControl const &carControl, Car &car, CarStateLastUpdate &lastState) const
{
    auto currentSteering = car.vehicle->getSteeringValue(0);
    auto currentAcceleration = car.vehicle->getWheelInfo(2).m_engineForce;
    auto currentBrake = car.vehicle->getWheelInfo(2).m_brake;
    
    auto steering = carControl.desiredSteering * car.maxSteering;
    auto acceleration = carControl.desiredAcceleration * car.maxAcceleration;
    auto brake = carControl.desiredBrake * car.maxBrake;

    float desiredSteering = std::clamp(steering, -car.maxSteering, car.maxSteering);
    float desiredAcceleration = std::clamp(acceleration, -(car.maxAcceleration * 0.5f), car.maxAcceleration);
    float desiredBrake = std::clamp(brake, 0.0f, car.maxBrake);

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

    lastState.speed = car.vehicle->getCurrentSpeedKmHour();
    lastState.direction = toGlm(car.vehicle->getForwardVector());
    lastState.position = toGlm(car.vehicle->getChassisWorldTransform().getOrigin());
}

void CarKeyboardSystem::update(entt::registry &registry, float delta, entt::entity ent, KeyboardInput const &input, CarControl &carControl) const
{
    if (input.keys[GLFW_KEY_RIGHT]) {
        carControl.desiredSteering = -1.f;
    }
    else if (input.keys[GLFW_KEY_LEFT]) {
        carControl.desiredSteering = 1.f;
    }
    else {
        carControl.desiredSteering = 0.0f;
    }

    if (input.keys[GLFW_KEY_UP]) {
        carControl.desiredAcceleration = 1.0f;
    }
    else if (input.keys[GLFW_KEY_DOWN]) {
        carControl.desiredAcceleration = -1.0f;
    }
    else {
        carControl.desiredAcceleration = 0.0f;
    }

    if (input.keys[GLFW_KEY_SPACE]) {
        carControl.desiredBrake = 1.0f;
    }
    else {
        carControl.desiredBrake = 0.0f;
    }
}

void CarJoystickSystem::update(entt::registry &registry, float delta, entt::entity ent, GamepadInput const &input, CarStateLastUpdate const &lastState, CarControl &carControl) const
{
    carControl.desiredAcceleration = 0.0f;
    carControl.desiredBrake = 0.0f;

    carControl.desiredSteering = -(input.leftStick.x);
    // If we are going forwards, the right trigger controls acceleration
    if (lastState.speed >= 0.0f) {
        carControl.desiredAcceleration = input.rightTrigger * 1.0f;
        carControl.desiredBrake = input.leftTrigger * 1.0f;
    }
    else {
        carControl.desiredAcceleration = -input.leftTrigger * 1.0f;
        carControl.desiredBrake = input.rightTrigger * 1.0f;
    }

    // TODO: Right now you have to stop completely before you can reverse. fix this
}
