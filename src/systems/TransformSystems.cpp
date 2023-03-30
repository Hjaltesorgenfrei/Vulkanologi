#include "TransformSystems.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// glm::mat4 fromCollisionObject(btCollisionObject* body) {
//     auto scale = body->getCollisionShape()->getLocalScaling();
//     auto transform = body->getWorldTransform();
//     glm::mat4 modelMatrix;
//     transform.getOpenGLMatrix(glm::value_ptr(modelMatrix));
//     return glm::scale(modelMatrix, glm::vec3(scale.x(), scale.y(), scale.z()));
// }

void SensorTransformSystem::update(entt::registry &registry, float delta, entt::entity ent, Sensor const &sensor, Transform &transform) const
{
    // transform.modelMatrix = fromCollisionObject(sensor.ghost);
}

void RigidBodySystem::update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const &body, Transform &transform) const
{
    glm::quat rotation(body.rotation.w, body.rotation.x, body.rotation.y, body.rotation.z);
    transform.modelMatrix = glm::translate(glm::mat4(1.0f), body.position);
    transform.modelMatrix = glm::scale(transform.modelMatrix, body.scale);
}

void CarTransformSystem::update(entt::registry &registry, float delta, entt::entity ent, Car const &car, Transform &transform) const
{
    // transform.modelMatrix = fromCollisionObject(car.vehicle->getRigidBody());
    // A nasty hack to make the car look like it's on the ground and not in the ground
    // transform.modelMatrix = glm::translate(transform.modelMatrix, glm::vec3(0, 0.5f, 0));
}

void TransformControlPointsSystem::update(entt::registry &registry, float delta, entt::entity ent, Transform const &transform, ControlPointPtr &controlPoint) const
{
    controlPoint.controlPoint->update(transform.modelMatrix);
}
