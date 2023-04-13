#include "TransformSystems.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void RigidBodySystem::update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const &body, Transform &transform) const
{
    transform.modelMatrix = body.getTransform();
}

void TransformControlPointsSystem::update(entt::registry &registry, float delta, entt::entity ent, Transform const &transform, ControlPointPtr &controlPoint) const
{
    controlPoint.controlPoint->update(transform.modelMatrix);
}

void PhysicsCamera::update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const& body, BehCamera &camera) const
{
    auto carPosition = body.position;
    auto carRotation = body.rotation;
    auto carVelocity = body.velocity;

    carRotation.y *= -1;
    auto forward = glm::vec3(0.0f, 0.0f, 1.0f) * carRotation; 

    auto speed = glm::length(glm::vec2(carVelocity.x, carVelocity.z));
    auto cameraPosition = carPosition + (forward * -15.0f) + glm::vec3(0.0f, 6.0f, 0.0f) + (speed * -0.2f * forward);
    auto cameraTarget = carPosition + (forward * 10.0f) + glm::vec3(0.0f, 2.0f, 0.0f);

    camera.setCameraPosition(cameraPosition, delta);
    camera.setTarget(cameraTarget, speed, delta);
}