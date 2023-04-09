#include "TransformSystems.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void RigidBodySystem::update(entt::registry &registry, float delta, entt::entity ent, PhysicsBody const &body, Transform &transform) const
{
    glm::quat rotation(body.rotation.x, body.rotation.y, body.rotation.z, body.rotation.w);
    transform.modelMatrix = glm::translate(glm::mat4(1.0f), body.position);
    transform.modelMatrix = glm::rotate(transform.modelMatrix, glm::angle(rotation), glm::axis(rotation));
    transform.modelMatrix = glm::scale(transform.modelMatrix, body.scale);
}

void TransformControlPointsSystem::update(entt::registry &registry, float delta, entt::entity ent, Transform const &transform, ControlPointPtr &controlPoint) const
{
    controlPoint.controlPoint->update(transform.modelMatrix);
}
