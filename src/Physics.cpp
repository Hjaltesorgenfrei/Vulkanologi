#include "Physics.hpp"

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
    // auto groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    // auto groundTransform = btTransform();
    // groundTransform.setIdentity();
    // groundTransform.setOrigin(btVector3(0, -1, 0));
    // auto groundMass = 0.f;
    // auto groundLocalInertia = btVector3(0, 0, 0);
    // auto groundMotionState = new btDefaultMotionState(groundTransform);
    // auto groundRigidBodyCI = btRigidBody::btRigidBodyConstructionInfo(groundMass, groundMotionState, groundShape, groundLocalInertia);
    // auto groundRigidBody = new btRigidBody(groundRigidBodyCI);
    // addBody(groundRigidBody);
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

void PhysicsWorld::addSensor(btGhostObject *ghost)
{
    dynamicsWorld->addCollisionObject(ghost, btBroadphaseProxy::SensorTrigger, btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::SensorTrigger);
}

void PhysicsWorld::removeSensor(btGhostObject *ghost)
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

#define CUBE_HALF_EXTENTS 1
float wheelWidth = 0.4f;
float wheelRadius = 0.5f;
int rightIndex = 0;
int upIndex = 1;
int forwardIndex = 2;
float wheelFriction = 1000;  //BT_LARGE_FLOAT;
float suspensionStiffness = 20.f;
float suspensionDamping = 2.3f;
float suspensionCompression = 4.4f;
float rollInfluence = 0.1f;  //1.0f;
btVector3 wheelDirectionCS0(0, -1, 0);
btVector3 wheelAxleCS(-1, 0, 0);
btScalar suspensionRestLength(1.f);

btRaycastVehicle* PhysicsWorld::createVehicle()
{
    btCollisionShape* chassisShape = new btBoxShape(btVector3(1.3f, 0.5f, 2.6f));
    btCompoundShape* compound = new btCompoundShape();
    btTransform localTrans;
	localTrans.setIdentity();
	//localTrans effectively shifts the center of mass with respect to the chassis
	localTrans.setOrigin(btVector3(0, 1, 0));
    compound->addChildShape(localTrans, chassisShape);
    btDefaultMotionState* myMotionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 2, 0)));
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
    float connectionHeight = 1.2f;

    bool isFrontWheel = true;

    //choose coordinate system
    vehicle->setCoordinateSystem(rightIndex, upIndex, forwardIndex);

    btVector3 connectionPointCS0(CUBE_HALF_EXTENTS - (0.3 * wheelWidth), connectionHeight, 2 * CUBE_HALF_EXTENTS - wheelRadius);

    vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);
    connectionPointCS0 = btVector3(-CUBE_HALF_EXTENTS + (0.3 * wheelWidth), connectionHeight, 2 * CUBE_HALF_EXTENTS - wheelRadius);

    vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);
    connectionPointCS0 = btVector3(-CUBE_HALF_EXTENTS + (0.3 * wheelWidth), connectionHeight, -2 * CUBE_HALF_EXTENTS + wheelRadius);
    isFrontWheel = false;
    vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);
    connectionPointCS0 = btVector3(CUBE_HALF_EXTENTS - (0.3 * wheelWidth), connectionHeight, -2 * CUBE_HALF_EXTENTS + wheelRadius);
    vehicle->addWheel(connectionPointCS0, wheelDirectionCS0, wheelAxleCS, suspensionRestLength, wheelRadius, tuning, isFrontWheel);


    for (int i = 0; i < vehicle->getNumWheels(); i++) {
        btWheelInfo& wheel = vehicle->getWheelInfo(i);
        wheel.m_suspensionStiffness = suspensionStiffness;
        wheel.m_wheelsDampingRelaxation = suspensionDamping;
        wheel.m_wheelsDampingCompression = suspensionCompression;
        wheel.m_frictionSlip = wheelFriction;
        wheel.m_rollInfluence = rollInfluence;
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

btRigidBody *PhysicsWorld::createWorldGeometry(const std::vector<btVector3> &vertices)
{
    btTriangleMesh* triangleMesh = new btTriangleMesh();
    for (int i = 0; i < vertices.size(); i += 3) {
        triangleMesh->addTriangle(vertices[i], vertices[i + 1], vertices[i + 2]);
    }
    btBvhTriangleMeshShape* shape = new btBvhTriangleMeshShape(triangleMesh, true);
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0, 0, 0));
    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0, motionState, shape);
    btRigidBody* body = new btRigidBody(rbInfo);
    // Dont debug draw because its slow
    body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_DISABLE_VISUALIZE_OBJECT);
    // Make kinematic
    body->setCollisionFlags(body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
    addBody(body);
    return body;
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