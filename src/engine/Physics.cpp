#include "Physics.hpp"

// X11 is stupid and defines None and Convex
#undef Convex
#undef None

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/StreamWrapper.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/OffsetCenterOfMassShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Vehicle/VehicleCollisionTester.h>
#include <Jolt/Physics/Vehicle/VehicleConstraint.h>
#include <Jolt/Physics/Vehicle/WheeledVehicleController.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Renderer/DebugRendererRecorder.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

// STL includes
#include <cstdarg>
#include <fstream>
#include <iostream>
#include <thread>

#include "Components.hpp"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store
// and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to
// double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...) {
	// Format the message
	va_list list;
	va_start(list, inFMT);
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), inFMT, list);
	va_end(list);

	// Print to the TTY
	cout << buffer << endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine) {
	// Print to the TTY
	cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << endl;

	// Breakpoint
	return true;
};

#endif  // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics
// simulation but only if you do collision testing).
namespace Layers {
static constexpr ObjectLayer NON_MOVING = 0;
static constexpr ObjectLayer MOVING = 1;
static constexpr ObjectLayer NUM_LAYERS = 2;
};  // namespace Layers

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter {
public:
	virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override {
		switch (inObject1) {
			case Layers::NON_MOVING:
				return inObject2 == Layers::MOVING;  // Non moving only collides with moving
			case Layers::MOVING:
				return true;  // Moving collides with everything
			default:
				JPH_ASSERT(false);
				return false;
		}
	}
};

// Each broadphase layer results in a separate bounding volume tree in the broad phase. You at least want to have
// a layer for non-moving and moving objects to avoid having to update a tree full of static objects every frame.
// You can have a 1-on-1 mapping between object layers and broadphase layers (like in this case) but if you have
// many object layers you'll be creating many broad phase trees, which is not efficient. If you want to fine tune
// your broadphase layers define JPH_TRACK_BROADPHASE_STATS and look at the stats reported on the TTY.
namespace BroadPhaseLayers {
static constexpr BroadPhaseLayer NON_MOVING(0);
static constexpr BroadPhaseLayer MOVING(1);
static constexpr uint NUM_LAYERS(2);
};  // namespace BroadPhaseLayers

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface {
public:
	BPLayerInterfaceImpl() {
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }

	virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override {
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override {
		switch ((BroadPhaseLayer::Type)inLayer) {
			case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
				return "NON_MOVING";
			case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
				return "MOVING";
			default:
				JPH_ASSERT(false);
				return "INVALID";
		}
	}
#endif  // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter {
public:
	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override {
		switch (inLayer1) {
			case Layers::NON_MOVING:
				return inLayer2 == BroadPhaseLayers::MOVING;
			case Layers::MOVING:
				return true;
			default:
				JPH_ASSERT(false);
				return false;
		}
	}
};

// An example contact listener
class MyContactListener : public ContactListener {
public:
	// See: ContactListener
	virtual ValidateResult OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset,
											 const CollideShapeResult &inCollisionResult) override {
		// cout << "Contact validate callback" << endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold,
								ContactSettings &ioSettings) override {
		// cout << "A contact was added" << endl;
	}

	virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold,
									ContactSettings &ioSettings) override {
		// cout << "A contact was persisted" << endl;
	}

	virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override {
		// cout << "A contact was removed" << endl;
	}
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener {
public:
	virtual void OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override {
		// cout << "A body got activated" << endl;
	}

	virtual void OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override {
		// cout << "A body went to sleep" << endl;
	}
};

DebugRendererRecorder *recorder;
ofstream renderer_file("performance_test.jor", ofstream::out | ofstream::binary | ofstream::trunc);
StreamOutWrapper renderer_stream(renderer_file);

PhysicsWorld::PhysicsWorld(entt::registry &registry) {
	// Register allocation hook
	RegisterDefaultAllocator();

	// Install callbacks
	Trace = TraceImpl;
	JPH_IF_ENABLE_ASSERTS(AssertFailed = AssertFailedImpl;)

	// Create a factory
	Factory::sInstance = new Factory();

	// Register all Jolt physics types
	RegisterTypes();

	// We need a temp allocator for temporary allocations during the physics update. We're
	// pre-allocating 10 MB to avoid having to do allocations during the physics update.
	// B.t.w. 10 MB is way too much for this example but it is a typical value you can use.
	// If you don't want to pre-allocate you can also use TempAllocatorMalloc to fall back to
	// malloc / free.
	tempAllocator = std::make_unique<TempAllocatorImpl>(10 * 1024 * 1024);

	// We need a job system that will execute physics jobs on multiple threads. Typically
	// you would implement the JobSystem interface yourself and let Jolt Physics run on top
	// of your own job scheduler. JobSystemThreadPool is an example implementation.
	jobSystem =
		std::make_unique<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get
	// an error. Note: This value is low because this is a simple test. For a real project use something in the order of
	// 65536.
	const uint cMaxBodies = 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the
	// default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this
	// buffer too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is
	// slightly less efficient. Note: This value is low because this is a simple test. For a real project use something
	// in the order of 65536.
	const uint cMaxBodyPairs = 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are
	// detected than this number then these contacts will be ignored and bodies will start interpenetrating / fall
	// through the world. Note: This value is low because this is a simple test. For a real project use something in the
	// order of 10240.
	const uint cMaxContactConstraints = 1024;

	// Create mapping table from object layer to broadphase layer
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	bpLayerInterfaceImpl = std::make_unique<BPLayerInterfaceImpl>();

	// Create class that filters object vs broadphase layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	objectVsBroadPhaseLayerFilterImpl = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();

	// Create class that filters object vs object layers
	// Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
	objectLayerPairFilterImpl = std::make_unique<ObjectLayerPairFilterImpl>();

	// Now we can create the actual physics system.
	physicsSystem = std::make_unique<PhysicsSystem>();
	physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *bpLayerInterfaceImpl,
						*objectVsBroadPhaseLayerFilterImpl, *objectLayerPairFilterImpl);
	physicsSystem->SetGravity(Vec3(0, -9.81f, 0));
	// A body activation listener gets notified when bodies activate and go to sleep
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	bodyActivationListener = std::make_unique<MyBodyActivationListener>();
	physicsSystem->SetBodyActivationListener(bodyActivationListener.get());

	// A contact listener gets notified when bodies (are about to) collide, and when they separate again.
	// Note that this is called from a job so whatever you do here needs to be thread safe.
	// Registering one is entirely optional.
	contactListener = std::make_unique<MyContactListener>();
	physicsSystem->SetContactListener(contactListener.get());

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision
	// detection performance (it's pointless here because we only have 2 bodies). You should definitely not call this
	// every frame or when e.g. streaming in a new level section as it is an expensive operation. Instead insert all new
	// objects in batches instead of 1 at a time to keep the broad phase efficient.
	physicsSystem->OptimizeBroadPhase();
	recorder = new DebugRendererRecorder(renderer_stream);
	bodyInterface = &physicsSystem->GetBodyInterface();

	// Setup up destructors for components
	registry.on_destroy<PhysicsBody>().connect<&PhysicsWorld::onPhysicsBodyDestroyed>(this);
	registry.on_destroy<CarPhysics>().connect<&PhysicsWorld::onCarPhysicsDestroyed>(this);
}

// Hidden struct which is only used by the physics system to interpolate the state of bodies.
struct InterpolatingBody {
	PhysicsBody current;
	PhysicsBody next;
};

void PhysicsWorld::onPhysicsBodyDestroyed(entt::registry &registry, entt::entity entity) {
	if (auto body = registry.try_get<PhysicsBody>(entity)) {
		removeBody(body->bodyID);
	}
	registry.remove<InterpolatingBody>(entity);
}

void PhysicsWorld::onCarPhysicsDestroyed(entt::registry &registry, entt::entity entity) {
	if (auto body = registry.try_get<CarPhysics>(entity)) {
		removeCar(body->constraint);
	}
}

PhysicsWorld::~PhysicsWorld() {
	for (auto id : bodies) {
		bodyInterface->RemoveBody(id);
		bodyInterface->DestroyBody(id);
	}

	// Unregisters all types with the factory and cleans up the default material
	UnregisterTypes();

	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;
}

void PhysicsWorld::addBody(entt::registry &registry, entt::entity entity, IDType id) {
	registry.emplace<PhysicsBody>(entity, getBody(id));
	registry.emplace<InterpolatingBody>(entity, getBody(id));
}

void PhysicsWorld::addFloor(entt::registry &registry, entt::entity entity, glm::vec3 position) {
	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape).
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));

	// Create the shape
	ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	ShapeRefC floor_shape =
		floor_shape_result
			.Get();  // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution /
	// friction.
	BodyCreationSettings floor_settings(floor_shape, RVec3(position.x, position.y, position.z), Quat::sIdentity(),
										EMotionType::Static, Layers::NON_MOVING);
	floor_settings.mFriction = 1.0f;
	// Create the actual rigid body
	auto floor_id = bodyInterface->CreateAndAddBody(
		floor_settings, EActivation::DontActivate);  // Note that if we run out of bodies this can return nullptr
	if (floor_id.IsInvalid()) {
		handleInvalidId("Failed to create floor", floor_id);
	}

	setUserData(floor_id, entity);
	bodies.push_back(floor_id);
}

void PhysicsWorld::addSphere(entt::registry &registry, entt::entity entity, glm::vec3 position, float radius,
							 bool isSensor) {
	// Now create a dynamic body to bounce on the floor
	// Note that this uses the shorthand version of creating and adding a body to the world
	BodyCreationSettings sphere_settings(new SphereShape(radius), RVec3(position.x, position.y, position.z),
										 Quat::sIdentity(), isSensor ? EMotionType::Static : EMotionType::Dynamic,
										 Layers::MOVING);
	sphere_settings.mIsSensor = isSensor;
	auto sphere_id = bodyInterface->CreateAndAddBody(sphere_settings, EActivation::Activate);
	if (sphere_id.IsInvalid()) {
		handleInvalidId("Failed to create sphere", sphere_id);
	}

	setUserData(sphere_id, entity);
	bodies.push_back(sphere_id);
	addBody(registry, entity, sphere_id);
}

IDType PhysicsWorld::addBox(entt::registry &registry, entt::entity entity, glm::vec3 position, glm::vec3 size) {
	BodyCreationSettings box_settings(new BoxShape(Vec3(size.x, size.y, size.z)),
									  RVec3(position.x, position.y, position.z), Quat::sIdentity(),
									  EMotionType::Dynamic, Layers::MOVING);
	auto box_id = bodyInterface->CreateAndAddBody(box_settings, EActivation::Activate);

	if (box_id.IsInvalid()) {
		handleInvalidId("Failed to create box", box_id);
	}

	setUserData(box_id, entity);
	bodies.push_back(box_id);
	addBody(registry, entity, box_id);
	return box_id;
}

void PhysicsWorld::addCar(entt::registry &registry, entt::entity entity, glm::vec3 positionIn) {
	const float wheel_radius = 0.3f;
	const float wheel_width = 0.1f;
	const float half_vehicle_length = 2.0f;
	const float half_vehicle_width = 0.9f;
	const float half_vehicle_height = 0.2f;

	float sInitialRollAngle = 0;
	float sMaxRollAngle = DegreesToRadians(60.0f);
	float sMaxSteeringAngle = DegreesToRadians(30.0f);
	int sCollisionMode = 2;
	bool sFourWheelDrive = false;
	bool sAntiRollbar = true;
	float sFrontCasterAngle = 0.0f;
	float sFrontKingPinAngle = 0.0f;
	float sFrontCamber = 0.0f;
	float sFrontToe = 0.0f;
	float sFrontSuspensionForwardAngle = 0.0f;
	float sFrontSuspensionSidewaysAngle = 0.0f;
	float sFrontSuspensionMinLength = 0.3f;
	float sFrontSuspensionMaxLength = 0.5f;
	float sFrontSuspensionFrequency = 1.5f;
	float sFrontSuspensionDamping = 0.5f;
	float sRearSuspensionForwardAngle = 0.0f;
	float sRearSuspensionSidewaysAngle = 0.0f;
	float sRearCasterAngle = 0.0f;
	float sRearKingPinAngle = 0.0f;
	float sRearCamber = 0.0f;
	float sRearToe = 0.0f;
	float sRearSuspensionMinLength = 0.3f;
	float sRearSuspensionMaxLength = 0.5f;
	float sRearSuspensionFrequency = 1.5f;
	float sRearSuspensionDamping = 0.5f;

	Body *mCarBody;                         ///< The vehicle
	VehicleConstraint *mVehicleConstraint;  ///< The vehicle constraint

	// Create collision testers
	auto collisionTester = new VehicleCollisionTesterCastCylinder(Layers::MOVING);

	// Create vehicle body
	RVec3 position(positionIn.x, positionIn.y, positionIn.z);
	RefConst<Shape> car_shape = OffsetCenterOfMassShapeSettings(
									Vec3(0, -half_vehicle_height, 0),
									new BoxShape(Vec3(half_vehicle_width, half_vehicle_height, half_vehicle_length)))
									.Create()
									.Get();
	BodyCreationSettings car_body_settings(car_shape, position, Quat::sRotation(Vec3::sAxisZ(), sInitialRollAngle),
										   EMotionType::Dynamic, Layers::MOVING);
	car_body_settings.mOverrideMassProperties = EOverrideMassProperties::CalculateInertia;
	car_body_settings.mMassPropertiesOverride.mMass = 1500.0f;
	mCarBody = bodyInterface->CreateBody(car_body_settings);
	bodyInterface->AddBody(mCarBody->GetID(), EActivation::Activate);

	// Create vehicle constraint
	VehicleConstraintSettings vehicle;
	vehicle.mDrawConstraintSize = 0.1f;
	vehicle.mMaxPitchRollAngle = sMaxRollAngle;

	// Suspension direction
	Vec3 front_suspension_dir =
		Vec3(Tan(sFrontSuspensionSidewaysAngle), -1, Tan(sFrontSuspensionForwardAngle)).Normalized();
	Vec3 front_steering_axis = Vec3(-Tan(sFrontKingPinAngle), 1, -Tan(sFrontCasterAngle)).Normalized();
	Vec3 front_wheel_up = Vec3(Sin(sFrontCamber), Cos(sFrontCamber), 0);
	Vec3 front_wheel_forward = Vec3(-Sin(sFrontToe), 0, Cos(sFrontToe));
	Vec3 rear_suspension_dir =
		Vec3(Tan(sRearSuspensionSidewaysAngle), -1, Tan(sRearSuspensionForwardAngle)).Normalized();
	Vec3 rear_steering_axis = Vec3(-Tan(sRearKingPinAngle), 1, -Tan(sRearCasterAngle)).Normalized();
	Vec3 rear_wheel_up = Vec3(Sin(sRearCamber), Cos(sRearCamber), 0);
	Vec3 rear_wheel_forward = Vec3(-Sin(sRearToe), 0, Cos(sRearToe));
	Vec3 flip_x(-1, 1, 1);

	// Wheels, left front
	WheelSettingsWV *w1 = new WheelSettingsWV;
	w1->mPosition = Vec3(half_vehicle_width, -0.9f * half_vehicle_height, half_vehicle_length - 2.0f * wheel_radius);
	w1->mSuspensionDirection = front_suspension_dir;
	w1->mSteeringAxis = front_steering_axis;
	w1->mWheelUp = front_wheel_up;
	w1->mWheelForward = front_wheel_forward;
	w1->mSuspensionMinLength = sFrontSuspensionMinLength;
	w1->mSuspensionMaxLength = sFrontSuspensionMaxLength;
	w1->mSuspensionSpring.mFrequency = sFrontSuspensionFrequency;
	w1->mSuspensionSpring.mDamping = sFrontSuspensionDamping;
	w1->mMaxSteerAngle = sMaxSteeringAngle;
	w1->mMaxHandBrakeTorque = 0.0f;  // Front wheel doesn't have hand brake

	// Right front
	WheelSettingsWV *w2 = new WheelSettingsWV;
	w2->mPosition = Vec3(-half_vehicle_width, -0.9f * half_vehicle_height, half_vehicle_length - 2.0f * wheel_radius);
	w2->mSuspensionDirection = flip_x * front_suspension_dir;
	w2->mSteeringAxis = flip_x * front_steering_axis;
	w2->mWheelUp = flip_x * front_wheel_up;
	w2->mWheelForward = flip_x * front_wheel_forward;
	w2->mSuspensionMinLength = sFrontSuspensionMinLength;
	w2->mSuspensionMaxLength = sFrontSuspensionMaxLength;
	w2->mSuspensionSpring.mFrequency = sFrontSuspensionFrequency;
	w2->mSuspensionSpring.mDamping = sFrontSuspensionDamping;
	w2->mMaxSteerAngle = sMaxSteeringAngle;
	w2->mMaxHandBrakeTorque = 0.0f;  // Front wheel doesn't have hand brake

	// Left rear
	WheelSettingsWV *w3 = new WheelSettingsWV;
	w3->mPosition = Vec3(half_vehicle_width, -0.9f * half_vehicle_height, -half_vehicle_length + 2.0f * wheel_radius);
	w3->mSuspensionDirection = rear_suspension_dir;
	w3->mSteeringAxis = rear_steering_axis;
	w3->mWheelUp = rear_wheel_up;
	w3->mWheelForward = rear_wheel_forward;
	w3->mSuspensionMinLength = sRearSuspensionMinLength;
	w3->mSuspensionMaxLength = sRearSuspensionMaxLength;
	w3->mSuspensionSpring.mFrequency = sRearSuspensionFrequency;
	w3->mSuspensionSpring.mDamping = sRearSuspensionDamping;
	w3->mMaxSteerAngle = 0.0f;

	// Right rear
	WheelSettingsWV *w4 = new WheelSettingsWV;
	w4->mPosition = Vec3(-half_vehicle_width, -0.9f * half_vehicle_height, -half_vehicle_length + 2.0f * wheel_radius);
	w4->mSuspensionDirection = flip_x * rear_suspension_dir;
	w4->mSteeringAxis = flip_x * rear_steering_axis;
	w4->mWheelUp = flip_x * rear_wheel_up;
	w4->mWheelForward = flip_x * rear_wheel_forward;
	w4->mSuspensionMinLength = sRearSuspensionMinLength;
	w4->mSuspensionMaxLength = sRearSuspensionMaxLength;
	w4->mSuspensionSpring.mFrequency = sRearSuspensionFrequency;
	w4->mSuspensionSpring.mDamping = sRearSuspensionDamping;
	w4->mMaxSteerAngle = 0.0f;

	vehicle.mWheels = {w1, w2, w3, w4};

	for (WheelSettings *w : vehicle.mWheels) {
		w->mRadius = wheel_radius;
		w->mWidth = wheel_width;
	}

	WheeledVehicleControllerSettings *controller = new WheeledVehicleControllerSettings;
	vehicle.mController = controller;

	// Differential
	controller->mDifferentials.resize(sFourWheelDrive ? 2 : 1);
	controller->mDifferentials[0].mLeftWheel = 0;
	controller->mDifferentials[0].mRightWheel = 1;
	if (sFourWheelDrive) {
		controller->mDifferentials[1].mLeftWheel = 2;
		controller->mDifferentials[1].mRightWheel = 3;

		// Split engine torque
		controller->mDifferentials[0].mEngineTorqueRatio = controller->mDifferentials[1].mEngineTorqueRatio = 0.5f;
	}

	// Anti rollbars
	if (sAntiRollbar) {
		vehicle.mAntiRollBars.resize(2);
		vehicle.mAntiRollBars[0].mLeftWheel = 0;
		vehicle.mAntiRollBars[0].mRightWheel = 1;
		vehicle.mAntiRollBars[1].mLeftWheel = 2;
		vehicle.mAntiRollBars[1].mRightWheel = 3;
	}

	mVehicleConstraint = new VehicleConstraint(*mCarBody, vehicle);
	mVehicleConstraint->SetVehicleCollisionTester(collisionTester);
	physicsSystem->AddConstraint(mVehicleConstraint);
	physicsSystem->AddStepListener(mVehicleConstraint);
	addBody(registry, entity, mCarBody->GetID());
	registry.emplace<CarPhysics>(entity, CarPhysics{mVehicleConstraint});
}

void PhysicsWorld::addMesh(entt::registry &registry, entt::entity entity, std::vector<glm::vec3> &vertices,
						   std::vector<uint32_t> &indices, glm::vec3 position, glm::vec3 scale, MotionType motionType) {
	VertexList inVertices;
	inVertices.reserve(vertices.size());
	IndexedTriangleList inTriangles;
	inTriangles.reserve(indices.size());

	for (int i = 0; i < vertices.size(); i++) {
		inVertices.emplace_back(vertices[i].x, vertices[i].y, vertices[i].z);
	}

	for (int i = 0; i < indices.size(); i += 3) {
		inTriangles.emplace_back(indices[i], indices[i + 1], indices[i + 2]);
	}

	BodyCreationSettings mesh_settings;
	RVec3 pos(position.x, position.y, position.z);
	auto meshShapeSettings = new MeshShapeSettings(inVertices, inTriangles);

	if (motionType == MotionType::Kinematic) {
		mesh_settings =
			BodyCreationSettings(meshShapeSettings, pos, Quat::sIdentity(), EMotionType::Kinematic, Layers::MOVING);
		mesh_settings.mOverrideMassProperties = EOverrideMassProperties::MassAndInertiaProvided;
		mesh_settings.mMassPropertiesOverride = MassProperties();
		mesh_settings.mMassPropertiesOverride.mMass = 1.0f;
	} else  // motionType == Dynamic for meshes is not supported by Jolt
	{
		mesh_settings =
			BodyCreationSettings(meshShapeSettings, pos, Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
		mesh_settings.mFriction = 0.7f;
	}

	auto mesh_id = bodyInterface->CreateAndAddBody(mesh_settings, EActivation::Activate);

	bodyInterface->SetAngularVelocity(mesh_id, Vec3(0.0f, 0.1f, 0.0f));

	if (mesh_id.IsInvalid()) {
		handleInvalidId("Failed to create mesh", mesh_id);
	}

	setUserData(mesh_id, entity);
	bodies.push_back(mesh_id);
	setScale(mesh_id, scale);
	addBody(registry, entity, mesh_id);
}

void PhysicsWorld::removeCar(JPH::VehicleConstraint *constraint) {
	physicsSystem->RemoveStepListener(constraint);
	physicsSystem->RemoveConstraint(constraint);
}

void PhysicsWorld::rayPick(glm::vec3 origin, glm::vec3 direction, float maxDistance,
						   std::function<void(entt::entity entity)> callback) {
	auto &narrowPhase = physicsSystem->GetNarrowPhaseQueryNoLock();
	RRayCast rayCast;
	rayCast.mOrigin = RVec3(origin.x, origin.y, origin.z);
	rayCast.mDirection = RVec3(direction.x, direction.y, direction.z) * maxDistance;
	RayCastResult result;
	if (narrowPhase.CastRay(rayCast, result)) {
		auto entity = getUserData(result.mBodyID);
		callback(entity);
	}
}

std::vector<std::pair<glm::vec3, glm::vec3>> PhysicsWorld::debugDraw() {
	BodyManager::DrawSettings settings;
	settings.mDrawBoundingBox = true;
	physicsSystem->DrawBodies(settings, recorder);
	recorder->EndFrame();
	return {};
}

void PhysicsWorld::removeBody(IDType bodyID) {
	bodyInterface->RemoveBody(bodyID);
	bodyInterface->DestroyBody(bodyID);
}

PhysicsBody PhysicsWorld::getBody(IDType bodyID) {
	return {
		bodyID,           getMotionType(bodyID),     getPosition(bodyID),        getRotation(bodyID),
		getScale(bodyID), getLinearVelocity(bodyID), getAngularVelocity(bodyID),
	};
}

void PhysicsWorld::getBody(IDType bodyID, PhysicsBody &body) {
	body.bodyID = bodyID;
	body.position = getPosition(bodyID);
	body.rotation = getRotation(bodyID);
	body.scale = getScale(bodyID);
	body.linearVelocity = getLinearVelocity(bodyID);
	body.angularVelocity = getAngularVelocity(bodyID);
}

void PhysicsWorld::updateBody(IDType bodyID, PhysicsBody body) {
	setPosition(bodyID, body.position);
	setRotation(bodyID, body.rotation);
	setScale(bodyID, body.scale);
	setLinearVelocity(bodyID, body.linearVelocity);
	setAngularVelocity(bodyID, body.linearVelocity);
}

MotionType PhysicsWorld::getMotionType(IDType bodyID) {
	auto motion_type = bodyInterface->GetMotionType(bodyID);
	switch (motion_type) {
		case EMotionType::Static:
			return MotionType::Static;
		case EMotionType::Dynamic:
			return MotionType::Dynamic;
		case EMotionType::Kinematic:
			return MotionType::Kinematic;
		default:
			return MotionType::Static;
	}
}

glm::vec3 PhysicsWorld::getPosition(IDType bodyID) {
	RVec3 position = bodyInterface->GetCenterOfMassPosition(bodyID);
	return glm::vec3(position.GetX(), position.GetY(), position.GetZ());
}

glm::quat PhysicsWorld::getRotation(IDType bodyID) {
	Quat quat = bodyInterface->GetRotation(bodyID);
	return glm::quat(quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ());
}

glm::vec3 PhysicsWorld::getScale(IDType bodyID) {
	const Shape *shape = bodyInterface->GetShape(bodyID);
	if (shape->GetSubType() == EShapeSubType::Scaled) {
		// Might not always be valid, as the more decorations can be on the shape
		// In this case we would have to descent down the shapes and add the scale up.
		auto scaledShape = static_cast<const ScaledShape *>(shape);
		auto scale = scaledShape->GetScale();
		return glm::vec3(scale.GetX(), scale.GetY(), scale.GetZ());
	}
	return glm::vec3(1, 1, 1);
}

glm::vec3 PhysicsWorld::getLinearVelocity(IDType bodyID) {
	Vec3 velocity = bodyInterface->GetLinearVelocity(bodyID);
	return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
}

glm::vec3 PhysicsWorld::getAngularVelocity(IDType bodyID) {
	Vec3 velocity = bodyInterface->GetAngularVelocity(bodyID);
	return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
}

void PhysicsWorld::setPosition(IDType bodyID, glm::vec3 position) {
	// TODO: Maybe bool for if it should activate?
	bodyInterface->SetPosition(bodyID, RVec3(position.x, position.y, position.z), EActivation::Activate);
}

void PhysicsWorld::setRotation(IDType bodyID, glm::quat rotation) {
	bodyInterface->SetRotation(bodyID, Quat(rotation.w, rotation.x, rotation.y, rotation.z), EActivation::Activate);
}

void PhysicsWorld::setScale(IDType bodyID, glm::vec3 scale) {
	auto &body_interface = physicsSystem->GetBodyLockInterfaceNoLock();
	BodyLockWrite lock(body_interface, bodyID);
	auto result = lock.GetBody().GetShape()->ScaleShape(Vec3Arg(scale.x, scale.y, scale.z));
	if (result.HasError()) {
		handleInvalidId("Failed to scale body", bodyID);
	}
	bodyInterface->SetShape(bodyID, result.Get(), false, EActivation::Activate);
}

void PhysicsWorld::setLinearVelocity(IDType bodyID, glm::vec3 velocity) {
	bodyInterface->SetLinearVelocity(bodyID, Vec3(velocity.x, velocity.y, velocity.z));
}

void PhysicsWorld::setAngularVelocity(IDType bodyID, glm::vec3 velocity) {
	bodyInterface->SetAngularVelocity(bodyID, Vec3(velocity.x, velocity.y, velocity.z));
}

void PhysicsWorld::addForce(IDType bodyID, glm::vec3 force) {
	bodyInterface->AddForce(bodyID, Vec3(force.x, force.y, force.z));
}

void PhysicsWorld::addTorque(IDType bodyID, glm::vec3 torque) {
	bodyInterface->AddTorque(bodyID, Vec3(torque.x, torque.y, torque.z));
}

void PhysicsWorld::setUserData(IDType bodyID, entt::entity entity) {
	auto &body_interface = physicsSystem->GetBodyLockInterface();
	JPH::BodyLockWrite lock(body_interface, bodyID);
	lock.GetBody().SetUserData(static_cast<uint64_t>(entity));
}

entt::entity PhysicsWorld::getUserData(IDType bodyID) {
	return static_cast<entt::entity>(bodyInterface->GetUserData(bodyID));
}

void PhysicsWorld::handleInvalidId(std::string error, IDType bodyID) {
	std::cout << error << std::endl;
}

void PhysicsWorld::update(float dt, entt::registry &registry) {
	accumulator += dt;
	while (accumulator >= cDeltaTime) {
		accumulator -= cDeltaTime;

		// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep
		// the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
		const int cCollisionSteps = 1;

		registry.view<CarPhysics, CarControl>().each(
			[this](entt::entity entity, CarPhysics &carPhysics, CarControl &carControl) {
				if (carControl.desiredAcceleration != 0.0f || carControl.desiredBrake != 0.0f ||
					carControl.desiredSteering != 0.0f) {
					auto id = carPhysics.constraint->GetVehicleBody()->GetID();
					bodyInterface->ActivateBody(id);
				}
			});

		registry.view<InterpolatingBody>().each(
			[this](entt::entity entity, InterpolatingBody &body) { this->getBody(body.current.bodyID, body.current); });

		// Step the world
		physicsSystem->Update(cDeltaTime, cCollisionSteps, tempAllocator.get(), jobSystem.get());

		registry.view<InterpolatingBody>().each(
			[this](entt::entity entity, InterpolatingBody &body) { this->getBody(body.current.bodyID, body.next); });
	}

	float ratio = accumulator / cDeltaTime;

	registry.view<PhysicsBody, InterpolatingBody>().each([this, ratio](entt::entity entity, PhysicsBody &body,
																	   InterpolatingBody &interpolation) {
		body.position = glm::mix(interpolation.current.position, interpolation.next.position, ratio);
		body.rotation = glm::slerp(interpolation.current.rotation, interpolation.next.rotation, ratio);
		body.linearVelocity = glm::mix(interpolation.current.linearVelocity, interpolation.next.linearVelocity, ratio);
		body.angularVelocity = glm::mix(interpolation.current.angularVelocity, interpolation.next.angularVelocity, ratio);
		body.scale = glm::mix(interpolation.current.scale, interpolation.next.scale, ratio);
	});
}