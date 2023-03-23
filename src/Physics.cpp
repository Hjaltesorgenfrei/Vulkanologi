#include "Physics.h"

// Include btGhostObject header

#include <iostream>

PhysicsWorld::PhysicsWorld()
{
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    overlappingPairCache = new btDbvtBroadphase();
    solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));

    // Add debug drawing
    debugDrawer = new DebugDrawer();
    dynamicsWorld->setDebugDrawer(debugDrawer);

    // Create a ground plane
    auto groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    auto groundTransform = btTransform();
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -1, 0));
    auto groundMass = 0.f;
    auto groundLocalInertia = btVector3(0, 0, 0);
    auto groundMotionState = new btDefaultMotionState(groundTransform);
    auto groundRigidBodyCI = btRigidBody::btRigidBodyConstructionInfo(groundMass, groundMotionState, groundShape, groundLocalInertia);
    auto groundRigidBody = new btRigidBody(groundRigidBodyCI);
    addBody(groundRigidBody);

    // Create a second plane to test collision
    auto groundShape2 = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    auto groundTransform2 = btTransform();
    groundTransform2.setIdentity();
    groundTransform2.setOrigin(btVector3(0, -1.25, 0));
    auto groundMass2 = 0.f;
    auto groundLocalInertia2 = btVector3(0, 0, 0);
    auto groundMotionState2 = new btDefaultMotionState(groundTransform2);
    auto groundRigidBodyCI2 = btRigidBody::btRigidBodyConstructionInfo(groundMass2, groundMotionState2, groundShape2, groundLocalInertia2);
    auto groundRigidBody2 = new btRigidBody(groundRigidBodyCI2);
    addBody(groundRigidBody2);
}

PhysicsWorld::~PhysicsWorld()
{
    delete debugDrawer;
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
}

void PhysicsWorld::update(float dt)
{
    dynamicsWorld->stepSimulation(dt, 10);
}

void PhysicsWorld::addBody(btRigidBody *body)
{
    dynamicsWorld->addRigidBody(body);
}

void PhysicsWorld::removeBody(btRigidBody *body)
{
    dynamicsWorld->removeRigidBody(body);
}

void PhysicsWorld::addGhost(btGhostObject *ghost)
{
    dynamicsWorld->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::SensorTrigger);
}

void PhysicsWorld::removeGhost(btGhostObject *ghost)
{
    dynamicsWorld->removeCollisionObject(ghost);
}

void PhysicsWorld::closestRay(const btVector3 rayFromWorld, const btVector3 rayToWorld, RayCallback callback)
{
    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);
    dynamicsWorld->rayTest(rayFromWorld, rayToWorld, rayCallback);
    if (rayCallback.hasHit()) {
        btVector3 hitPoint = rayCallback.m_hitPointWorld;
        btVector3 hitNormal = rayCallback.m_hitNormalWorld;
        callback(rayCallback.m_collisionObject, hitPoint, hitNormal);
    }
}

btRaycastVehicle* PhysicsWorld::createVehicle()
{
    // get 2 random values for x and z position of the vehicle between -5 and 5
    float x = static_cast<float>((rand() % 10) - 5);
    float z = static_cast<float>((rand() % 10) - 5);

    btCollisionShape* chassisShape = new btBoxShape(btVector3(1, 1, 1.5));
    btCompoundShape* compound = new btCompoundShape();
    btTransform localTrans;
    localTrans.setIdentity();
    localTrans.setOrigin(btVector3(0, 1, 0));
    compound->addChildShape(localTrans, chassisShape);
    btDefaultMotionState* myMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(x, 2, z)));
    btScalar mass = 800;
    btVector3 localInertia(0, 0, 0);
    compound->calculateLocalInertia(mass, localInertia);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, compound, localInertia);
    btRigidBody* chassis = new btRigidBody(rbInfo);
    chassis->setActivationState(DISABLE_DEACTIVATION);
    dynamicsWorld->addRigidBody(chassis);
    btRaycastVehicle::btVehicleTuning tuning;
    btVehicleRaycaster* raycaster = new btDefaultVehicleRaycaster(dynamicsWorld);
    btRaycastVehicle* vehicle = new btRaycastVehicle(tuning, chassis, raycaster);
    dynamicsWorld->addAction(vehicle);
    btVector3 wheelDirectionCS0(0, -1, 0);
    btVector3 wheelAxleCS(-1, 0, 0);
    bool isFrontWheel = true;
    btScalar suspensionRestLength(0.6f);
    btScalar wheelRadius(0.5f);
    vehicle->addWheel(btVector3(-1, -0.001f, 2), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);
    vehicle->addWheel(btVector3(1, -0.001f, 2), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);
    isFrontWheel = false;
    vehicle->addWheel(btVector3(-1, -0.001f, -2), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);
    vehicle->addWheel(btVector3(1, -0.001f, -2), wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);
    for (int i = 0; i < vehicle->getNumWheels(); i++) {
        btWheelInfo& wheel = vehicle->getWheelInfo(i);
        wheel.m_suspensionStiffness = 20.f;
        wheel.m_wheelsDampingRelaxation = 2.3f;
        wheel.m_wheelsDampingCompression = 4.4f;
        wheel.m_frictionSlip = 1000.f;
        wheel.m_rollInfluence = 0.1f;
    }
    vehicle->setCoordinateSystem(0, 1, 2);

    return vehicle;
}

void PhysicsWorld::removeVehicle(btRaycastVehicle *vehicle)
{
    dynamicsWorld->removeAction(vehicle);
    dynamicsWorld->removeRigidBody(vehicle->getRigidBody());
    delete vehicle;
}

std::vector<Path> PhysicsWorld::getDebugLines() const
{
    debugDrawer->clearLines();
    dynamicsWorld->debugDrawWorld();
    return debugDrawer->paths;
}

void DebugDrawer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) {
    auto path = LinePath(glm::vec3(from.x(), from.y(), from.z()), glm::vec3(to.x(), to.y(), to.z()), glm::vec3(color.x(), color.y(), color.z()));
    paths.emplace_back(path);
}

void DebugDrawer::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) {
    auto path = LinePath(glm::vec3(PointOnB.x(), PointOnB.y(), PointOnB.z()), glm::vec3(PointOnB.x(), PointOnB.y(), PointOnB.z()) + glm::vec3(normalOnB.x(), normalOnB.y(), normalOnB.z()) * distance, glm::vec3(color.x(), color.y(), color.z()));
    paths.emplace_back(path);
}

void DebugDrawer::reportErrorWarning(const char *warningString) {
    std::cerr << "Warning: " << warningString << std::endl;
}

void DebugDrawer::draw3dText(const btVector3 &location, const char *textString) {
    std::cerr << "Text: " << textString << std::endl;
}

void DebugDrawer::setDebugMode(int debugMode) {
    std::cerr << "Debug mode: " << debugMode << std::endl;
}

int DebugDrawer::getDebugMode() const {
    return DBG_DrawWireframe;
}

// clear lines
void DebugDrawer::clearLines() {
    paths.clear();
}