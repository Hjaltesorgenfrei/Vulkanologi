#pragma once 
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct ControllerSystem : System<ControllerSystem, Reads<>, Writes<ControllerInput>, Others<>>
{
    void update(entt::registry &registry, float delta, entt::entity ent, ControllerInput &input) const;
};