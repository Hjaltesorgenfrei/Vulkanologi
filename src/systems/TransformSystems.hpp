#pragma once 
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct SensorTransformSystem : System<SensorTransformSystem, Reads<Sensor>, Writes<Transform>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, Sensor const &sensor, Transform &transform) const;
};

struct RigidBodySystem : System<RigidBodySystem, Reads<RigidBody>, Writes<Transform>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, RigidBody const &rigidBody, Transform &transform) const;
};

struct CarTransformSystem : System<CarTransformSystem, Reads<Car>, Writes<Transform>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, Car const &car, Transform &transform) const;
};

struct TransformControlPointsSystem : System<TransformControlPointsSystem, Reads<Transform>, Writes<ControlPointPtr>, Others<>> {
    void update(entt::registry &registry, float delta, entt::entity ent, Transform const &transform, ControlPointPtr &controlPoint) const;
};
