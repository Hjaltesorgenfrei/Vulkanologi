#pragma once

#ifndef PHYSICS_H
#define PHYSICS_H

#include <functional>
#include <memory>

namespace JPH {
    class PhysicsSystem;
}

class BPLayerInterfaceImpl;
class ObjectVsBroadPhaseLayerFilterImpl;
class ObjectLayerPairFilterImpl;

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void update(float dt);
private:
    std::unique_ptr<JPH::PhysicsSystem> physicsSystem;
    std::unique_ptr<BPLayerInterfaceImpl> bpLayerInterfaceImpl;
    std::unique_ptr<ObjectVsBroadPhaseLayerFilterImpl> objectVsBroadPhaseLayerFilterImpl;
    std::unique_ptr<ObjectLayerPairFilterImpl> objectLayerPairFilterImpl;
};


#endif