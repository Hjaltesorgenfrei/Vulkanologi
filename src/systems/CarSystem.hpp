#pragma once
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct CarSystem : System<CarSystem, Reads<CarControl>, Writes<Car>, Others<CarStateLastUpdate>> {
    void update(entt::registry &registry, float delta, entt::entity ent, CarControl const &carControl, Car &car, CarStateLastUpdate &lastState) const;
};

struct CarKeyboardSystem : System<CarKeyboardSystem, Reads<KeyboardInput>, Writes<CarControl>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, KeyboardInput const &input, CarControl &carControl) const;
};

struct CarJoystickSystem : System<CarJoystickSystem, Reads<GamepadInput, CarStateLastUpdate>, Writes<CarControl>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, GamepadInput const &input, CarStateLastUpdate const &lastState, CarControl &carControl) const;
};
