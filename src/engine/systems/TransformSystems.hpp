#pragma once 
#include "../DependentSystem.hpp"
#include "../Components.hpp"
#include "../PhysicsBody.hpp"
#include <BehCamera.hpp>

struct RigidBodySystem : System<RigidBodySystem, Reads<PhysicsBody>, Writes<Transform>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const &body, Transform &transform) const;
};

struct TransformControlPointsSystem : System<TransformControlPointsSystem, Reads<Transform>, Writes<ControlPointPtr>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, Transform const &transform, ControlPointPtr &controlPoint) const;
};

struct PhysicsCamera : System<PhysicsCamera, Reads<PhysicsBody>, Writes<BehCamera>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const& body, BehCamera &camera) const;
};
