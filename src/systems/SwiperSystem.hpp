#pragma once
#pragma once 
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct SwiperSystem : System<SwiperSystem, Reads<Swiper>, Writes<RigidBody>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, Swiper const &swiper, RigidBody &body) const;
};