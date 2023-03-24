#pragma once 
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct ControllerSystem : Reads<>::Writes<ControllerInput>::Named<"ControllerSystem"> {
    virtual void run(float delta, ControllerInput& input) const override;
};