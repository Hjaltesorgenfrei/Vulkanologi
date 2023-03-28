#include "SwiperSystem.hpp"

void SwiperSystem::update(entt::registry &registry, float delta, entt::entity ent, Swiper const &swiper, RigidBody &body) const
{
    auto rigidBody = body.body;
    auto axis = swiper.axis;
    auto speed = swiper.speed;
    btTransform transform;
    rigidBody->getMotionState()->getWorldTransform(transform);
    auto position = transform.getOrigin();
    if (axis == Axis::X) {
        position.setX(position.getX() + speed * delta);
    } else if (axis == Axis::Y) {
        position.setY(position.getY() + speed * delta);
    } else if (axis == Axis::Z) {
        position.setZ(position.getZ() + speed * delta);
    }
    transform.setOrigin(position);
    rigidBody->getMotionState()->setWorldTransform(transform);
}