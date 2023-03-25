#pragma once 
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct ControllerSystem : WSystem<ControllerSystem, Writes<ControllerInput>>
{
    void update(entt::registry &registry, float delta, entt::entity ent, ControllerInput &input) const;
};