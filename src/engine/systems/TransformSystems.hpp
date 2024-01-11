#pragma once
#include "../Components.hpp"
#include "../DependentSystem.hpp"
#include "../PhysicsBody.hpp"

struct RigidBodySystem : System<RigidBodySystem, Reads<PhysicsBody>, Writes<Transform>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const &body,
				Transform &transform) const;
};

struct TransformControlPointsSystem
	: System<TransformControlPointsSystem, Reads<Transform>, Writes<ControlPointPtr>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, Transform const &transform,
				ControlPointPtr &controlPoint) const;
};

struct PhysicsCamera : System<PhysicsCamera, Reads<PhysicsBody>, Writes<Camera>, Others<>> {
	void update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const &body, Camera &camera) const;
};
