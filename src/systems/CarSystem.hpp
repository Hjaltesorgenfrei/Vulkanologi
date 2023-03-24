#pragma once
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct CarJoystickSystem : Reads<ControllerInput, CarStateLastUpdate>::Writes<CarControl>::Named<"CarJoystickSystem"> {
    virtual void run(float delta, ControllerInput input, CarStateLastUpdate lastState, CarControl& carControl) const override;
};

struct CarKeyboardSystem : Reads<KeyboardInput>::Writes<CarControl>::Named<"CarKeyboardSystem"> {
    virtual void run(float delta, KeyboardInput input, CarControl& carControl) const override;
};

struct CarSystem : Reads<CarControl>::Writes<Car>::Named<"CarSystem", CarStateLastUpdate> {
    virtual void run(float delta, CarControl carControl, Car& car, CarStateLastUpdate& lastState) const override;
};