#pragma once

#ifndef PHYSICS_H
#define PHYSICS_H

#include <functional>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <entt/entt.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include "PhysicsBody.hpp"

namespace JPH
{
    class PhysicsSystem;
    class JobSystem;
    class TempAllocator;
    class ContactListener;
    class BodyActivationListener;
    class BodyInterface;
}

// TODO: These might be able to swapped with name space interfaces instead which would be a bit cleaner as we can hide implementation details
class BPLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class ObjectLayerPairFilterImpl;

class PhysicsWorld
{
public:
    PhysicsWorld(entt::registry& registry);
    ~PhysicsWorld();

    void update(float dt, entt::registry& registry);

    void addFloor(entt::registry& registry, entt::entity entity, glm::vec3 position);
    void addSphere(entt::registry& registry, entt::entity entity, glm::vec3 position, float radius, bool isSensor = false);
    void addBox(entt::registry& registry, entt::entity entity, glm::vec3 position, glm::vec3 size);
    void addMesh(entt::registry& registry, entt::entity entity, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices, glm::vec3 position = glm::vec3(0), glm::vec3 scale = glm::vec3(1.f), MotionType motionType = MotionType::Static);
    void addCar(entt::registry& registry, entt::entity entity, glm::vec3 position);

    void rayPick(glm::vec3 origin, glm::vec3 direction, float maxDistance, std::function<void(entt::entity entity)> callback);

    std::vector<std::pair<glm::vec3, glm::vec3>> debugDraw();

    // TODO: Add that these modify the PhysicsBody.
    void setBodyPosition(IDType bodyID, glm::vec3 position);
    void setBodyRotation(IDType bodyID, glm::quat rotation);
    void setBodyScale(IDType bodyID, glm::vec3 scale);
    void setBodyVelocity(IDType bodyID, glm::vec3 velocity);

private:
    std::unique_ptr<JPH::PhysicsSystem> physicsSystem;
    std::unique_ptr<BPLayerInterfaceImpl> bpLayerInterfaceImpl;
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> objectVsBroadPhaseLayerFilterImpl;
    std::unique_ptr<ObjectLayerPairFilterImpl> objectLayerPairFilterImpl;
    std::unique_ptr<JPH::JobSystem> jobSystem;
    std::unique_ptr<JPH::TempAllocator> tempAllocator;
    std::unique_ptr<JPH::ContactListener> contactListener;
    std::unique_ptr<JPH::BodyActivationListener> bodyActivationListener;

    // Lifetime of bodies is managed by the physics system
    std::vector<IDType> bodies;

    const float cDeltaTime = 1.0f / 60.f;
    float accumulator = 0.0f;
    
    JPH::BodyInterface * bodyInterface = nullptr;

    void addBody(entt::registry& registry, entt::entity entity, IDType bodyID);

    PhysicsBody getBody(IDType bodyID);
    void getBody(IDType bodyID, PhysicsBody &body);
    void updateBody(IDType bodyID, PhysicsBody body);
    void removeBody(IDType bodyID);
    void removeCar(JPH::VehicleConstraint * constraint);

    void onPhysicsBodyDestroyed(entt::registry &registry, entt::entity entity);
    void onCarPhysicsDestroyed(entt::registry &registry, entt::entity entity);
    
    MotionType getMotionType(IDType bodyID);
    glm::vec3 getBodyPosition(IDType bodyID);
    glm::quat getBodyRotation(IDType bodyID);
    glm::vec3 getBodyScale(IDType bodyID);
    glm::vec3 getBodyVelocity(IDType bodyID);

    void setUserData(IDType bodyID, entt::entity entity);
    entt::entity getUserData(IDType bodyID);



    void handleInvalidId(std::string error, IDType bodyID);
};

#endif