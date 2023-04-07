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
    class BodyInterface;
    class VehicleConstraint;
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

struct CarPhysics {
    JPH::VehicleConstraint * constraint;
};

class PhysicsWorld
{
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void update(float dt, entt::registry& registry);

    PhysicsBody addFloor(entt::entity entity, glm::vec3 position);
    PhysicsBody addSphere(entt::entity entity, glm::vec3 position, float radius, bool isSensor = false);
    PhysicsBody addBox(entt::entity entity, glm::vec3 position, glm::vec3 size);
    PhysicsBody addMesh(entt::entity entity, std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices, glm::vec3 position = glm::vec3(0), MotionType motionType = MotionType::Static);
    std::pair<PhysicsBody, CarPhysics> addCar(entt::entity entity, glm::vec3 position);
    void removeCar(JPH::VehicleConstraint * constraint);

    void rayPick(glm::vec3 origin, glm::vec3 direction, float maxDistance, std::function<void(entt::entity entity)> callback);

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

    // We don't have a setTransform because it can not be seperated again.
    glm::mat4 getTransform(IDType bodyID);
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
    const float cDeltaTime = 1.0f / 200.f;
    float accumulator = 0.0f;
    
    JPH::BodyInterface * bodyInterface = nullptr;

    void setUserData(IDType bodyID, entt::entity entity);
    entt::entity getUserData(IDType bodyID);

    void handleInvalidId(std::string error, IDType bodyID);
};

#endif