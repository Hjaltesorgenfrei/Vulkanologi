#include "TransformSystems.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <BulletCollision/CollisionShapes/btCollisionShape.h>

glm::mat4 fromCollisionObject(btCollisionObject* body) {
    auto scale = body->getCollisionShape()->getLocalScaling();
    auto transform = body->getWorldTransform();
    glm::mat4 modelMatrix;
    transform.getOpenGLMatrix(glm::value_ptr(modelMatrix));
    return glm::scale(modelMatrix, glm::vec3(scale.x(), scale.y(), scale.z()));
}

void SensorTransformSystem::run(float delta, Sensor sensor, Transform &transform) const
{
    transform.modelMatrix = fromCollisionObject(sensor.ghost);
}

void RigidBodySystem::run(float delta, RigidBody rigidBody, Transform &transform) const
{
    transform.modelMatrix = fromCollisionObject(rigidBody.body);
}

void CarTransformSystem::run(float delta, Car car, Transform &transform) const
{
    transform.modelMatrix = fromCollisionObject(car.vehicle->getRigidBody());
}

void TransformControlPointsSystem::run(float delta, Transform transform, ControlPointPtr &controlPoint) const
{
    controlPoint.controlPoint->transform = transform.modelMatrix;
}
