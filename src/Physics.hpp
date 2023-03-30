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

namespace JPH
{
    class PhysicsSystem;
    class Body;
    class JobSystem;
    class TempAllocator;
    class ContactListener;
    class BodyActivationListener;
}

// TODO: These might be able to swapped with name space interfaces instead which would be a bit cleaner as we can hide implementation details
class BPLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class ObjectLayerPairFilterImpl;

typedef JPH::BodyID IDType;

enum class MotionType : uint8_t
{
    Static,
    Kinematic,
    Dynamic,
};

struct PhysicsBody
{
    IDType bodyID;
    MotionType physicsType;
    glm::vec3 position;
    glm::vec4 rotation;
    glm::vec3 scale;
    glm::vec3 velocity;
};

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void update(float dt, entt::registry& registry);

    PhysicsBody addFloor(entt::entity entity, glm::vec3 position);
    PhysicsBody addSphere(entt::entity entity, glm::vec3 position, float radius);
    PhysicsBody addBox(entt::entity entity, glm::vec3 position, glm::vec3 size);
    PhysicsBody addMesh(entt::entity entity, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices, glm::vec3 position = glm::vec3(0), MotionType motionType = MotionType::Static);

    std::vector<std::pair<glm::vec3, glm::vec3>> debugDraw();

    void removeBody(IDType bodyID);
    PhysicsBody getBody(IDType bodyID);
    void getBody(IDType bodyID, PhysicsBody &body);
    void updateBody(IDType bodyID, PhysicsBody body);
    
    // This should probably take in the entity ID instead of the body ID
    MotionType getMotionType(IDType bodyID);
    glm::vec3 getBodyPosition(IDType bodyID);
    glm::vec4 getBodyRotation(IDType bodyID);
    glm::vec3 getBodyScale(IDType bodyID);
    glm::vec3 getBodyVelocity(IDType bodyID);
    void setBodyPosition(IDType bodyID, glm::vec3 position);
    void setBodyRotation(IDType bodyID, glm::vec4 rotation);
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

    // We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.
    const float cDeltaTime = 1.0f / 60.0f;
    float accumulator = 0.0f;

    void setUserData(IDType bodyID, entt::entity entity);
    entt::entity getUserData(IDType bodyID);

    void handleInvalidId(std::string error, IDType bodyID);
};

#endif