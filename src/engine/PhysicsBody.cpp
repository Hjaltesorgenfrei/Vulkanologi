#include "PhysicsBody.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

glm::mat4 PhysicsBody::getTransform() const
{
    glm::mat4 transform;
    transform = glm::translate(glm::mat4(1.0f), position);
    transform = glm::rotate(transform, glm::angle(rotation), glm::axis(rotation));
    transform = glm::scale(transform, scale);
    return transform;
}