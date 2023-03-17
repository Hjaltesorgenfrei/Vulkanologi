#pragma once

#ifndef PHYSICS_H
#define PHYSICS_H

#include <btBulletDynamicsCommon.h>
#include "Path.h"

class DebugDrawer;

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void step(float dt);
    
    void addBody(btRigidBody* body);

    void removeBody(btRigidBody* body);

    void rayTest(const btVector3 rayFromWorld, const btVector3 rayToWorld);

    std::vector<Path> getDebugLines() const;

private:
    btDiscreteDynamicsWorld* dynamicsWorld;
    btCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;
    DebugDrawer* debugDrawer;
};

class DebugDrawer : public btIDebugDraw {
public:
    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override;

    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override;

    void reportErrorWarning(const char *warningString) override;

    void draw3dText(const btVector3 &location, const char *textString) override;

    void setDebugMode(int debugMode) override;

    int getDebugMode() const override;

    void clearLines() override;

    std::vector<Path> paths;

};

#endif