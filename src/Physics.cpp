#include "Physics.hpp"

// Include btGhostObject header

#include <iostream>

// X11 is stupid and defines None and Convex
#undef Convex
#undef None

// The Jolt headers don't include Jolt.h. Always include Jolt.h before including any other Jolt header.
// You can use Jolt.h in your precompiled header to speed up compilation.
#include <Jolt/Jolt.h>

// Jolt includes
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Renderer/DebugRendererRecorder.h>
#include <Jolt/Core/StreamWrapper.h>

// STL includes
#include <iostream>
#include <cstdarg>
#include <thread>
#include <fstream>

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS

// All Jolt symbols are in the JPH namespace
using namespace JPH;

// If you want your code to compile using single or double precision write 0.0_r to get a Real value that compiles to double or float depending if JPH_DOUBLE_PRECISION is set or not.
using namespace JPH::literals;

// We're also using STL classes in this example
using namespace std;

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char *inFMT, ...)
{
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
static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *inFile, uint inLine)
{
	// Print to the TTY
	cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << endl;

	// Breakpoint
	return true;
};

#endif // JPH_ENABLE_ASSERTS

// Layer that objects can be in, determines which other objects it can collide with
// Typically you at least want to have 1 layer for moving bodies and 1 layer for static bodies, but you can have more
// layers if you want. E.g. you could have a layer for high detail collision (which is not used by the physics simulation
// but only if you do collision testing).
namespace Layers
{
	static constexpr ObjectLayer NON_MOVING = 0;
	static constexpr ObjectLayer MOVING = 1;
	static constexpr ObjectLayer NUM_LAYERS = 2;
};

/// Class that determines if two object layers can collide
class ObjectLayerPairFilterImpl : public ObjectLayerPairFilter
{
public:
	virtual bool ShouldCollide(ObjectLayer inObject1, ObjectLayer inObject2) const override
	{
		switch (inObject1)
		{
		case Layers::NON_MOVING:
			return inObject2 == Layers::MOVING; // Non moving only collides with moving
		case Layers::MOVING:
			return true; // Moving collides with everything
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
namespace BroadPhaseLayers
{
	static constexpr BroadPhaseLayer NON_MOVING(0);
	static constexpr BroadPhaseLayer MOVING(1);
	static constexpr uint NUM_LAYERS(2);
};

// BroadPhaseLayerInterface implementation
// This defines a mapping between object and broadphase layers.
class BPLayerInterfaceImpl final : public BroadPhaseLayerInterface
{
public:
	BPLayerInterfaceImpl()
	{
		// Create a mapping table from object to broad phase layer
		mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
	}

	virtual uint GetNumBroadPhaseLayers() const override
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	virtual BroadPhaseLayer GetBroadPhaseLayer(ObjectLayer inLayer) const override
	{
		JPH_ASSERT(inLayer < Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
	virtual const char *GetBroadPhaseLayerName(BroadPhaseLayer inLayer) const override
	{
		switch ((BroadPhaseLayer::Type)inLayer)
		{
		case (BroadPhaseLayer::Type)BroadPhaseLayers::NON_MOVING:
			return "NON_MOVING";
		case (BroadPhaseLayer::Type)BroadPhaseLayers::MOVING:
			return "MOVING";
		default:
			JPH_ASSERT(false);
			return "INVALID";
		}
	}
#endif // JPH_EXTERNAL_PROFILE || JPH_PROFILE_ENABLED

private:
	BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

/// Class that determines if an object layer can collide with a broadphase layer
class ObjectVsBroadPhaseLayerFilterImpl : public ObjectVsBroadPhaseLayerFilter
{
public:
	virtual bool ShouldCollide(ObjectLayer inLayer1, BroadPhaseLayer inLayer2) const override
	{
		switch (inLayer1)
		{
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
class MyContactListener : public ContactListener
{
public:
	// See: ContactListener
	virtual ValidateResult OnContactValidate(const Body &inBody1, const Body &inBody2, RVec3Arg inBaseOffset, const CollideShapeResult &inCollisionResult) override
	{
		cout << "Contact validate callback" << endl;

		// Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
		return ValidateResult::AcceptAllContactsForThisBodyPair;
	}

	virtual void OnContactAdded(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
		cout << "A contact was added" << endl;
	}

	virtual void OnContactPersisted(const Body &inBody1, const Body &inBody2, const ContactManifold &inManifold, ContactSettings &ioSettings) override
	{
		cout << "A contact was persisted" << endl;
	}

	virtual void OnContactRemoved(const SubShapeIDPair &inSubShapePair) override
	{
		cout << "A contact was removed" << endl;
	}
};

// An example activation listener
class MyBodyActivationListener : public BodyActivationListener
{
public:
	virtual void OnBodyActivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "A body got activated" << endl;
	}

	virtual void OnBodyDeactivated(const BodyID &inBodyID, uint64 inBodyUserData) override
	{
		cout << "A body went to sleep" << endl;
	}
};

DebugRendererRecorder* recorder;
ofstream renderer_file("performance_test.jor", ofstream::out | ofstream::binary | ofstream::trunc);
StreamOutWrapper renderer_stream(renderer_file);

PhysicsWorld::PhysicsWorld()
{
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
	jobSystem = std::make_unique<JobSystemThreadPool>(cMaxPhysicsJobs, cMaxPhysicsBarriers, thread::hardware_concurrency() - 1);

	// This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodies = 1024;

	// This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
	const uint cNumBodyMutexes = 0;

	// This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
	// body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
	// too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
	const uint cMaxBodyPairs = 1024;

	// This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
	// number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
	// Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
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
	physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *bpLayerInterfaceImpl, *objectVsBroadPhaseLayerFilterImpl, *objectLayerPairFilterImpl);
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

	// Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
	// You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
	// Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
	physicsSystem->OptimizeBroadPhase();
	recorder = new DebugRendererRecorder(renderer_stream);
}

PhysicsWorld::~PhysicsWorld()
{
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();

	for (auto id : bodies)
	{
		body_interface.RemoveBody(id);
		body_interface.DestroyBody(id);
	}

	// Unregisters all types with the factory and cleans up the default material
	UnregisterTypes();

	// Destroy the factory
	delete Factory::sInstance;
	Factory::sInstance = nullptr;
}

PhysicsBody PhysicsWorld::addFloor(entt::entity entity, glm::vec3 position)
{
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();

	// Next we can create a rigid body to serve as the floor, we make a large box
	// Create the settings for the collision volume (the shape).
	// Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
	BoxShapeSettings floor_shape_settings(Vec3(100.0f, 1.0f, 100.0f));

	// Create the shape
	ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
	ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

	// Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
	BodyCreationSettings floor_settings(floor_shape, RVec3(position.x, position.y, position.z), Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);

	// Create the actual rigid body
	auto floor_id = body_interface.CreateAndAddBody(floor_settings, EActivation::DontActivate); // Note that if we run out of bodies this can return nullptr
	if (floor_id.IsInvalid())
	{
		handleInvalidId("Failed to create floor", floor_id);
	}

	setUserData(floor_id, entity);
	bodies.push_back(floor_id);
	return getBody(floor_id);
}

PhysicsBody PhysicsWorld::addSphere(entt::entity entity, glm::vec3 position, float radius)
{
	// The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
	// variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();

	// Now create a dynamic body to bounce on the floor
	// Note that this uses the shorthand version of creating and adding a body to the world
	BodyCreationSettings sphere_settings(new SphereShape(radius), RVec3(position.x, position.y, position.z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
	auto sphere_id = body_interface.CreateAndAddBody(sphere_settings, EActivation::Activate);
	if (sphere_id.IsInvalid())
	{
		handleInvalidId("Failed to create sphere", sphere_id);
	}

	setUserData(sphere_id, entity);
	bodies.push_back(sphere_id);
	return getBody(sphere_id);
}

PhysicsBody PhysicsWorld::addBox(entt::entity entity, glm::vec3 position, glm::vec3 size)
{
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();

	BodyCreationSettings box_settings(new BoxShape(Vec3(size.x, size.y, size.z)), RVec3(position.x, position.y, position.z), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
	auto box_id = body_interface.CreateAndAddBody(box_settings, EActivation::Activate);

	if (box_id.IsInvalid())
	{
		handleInvalidId("Failed to create box", box_id);
	}

	setUserData(box_id, entity);
	bodies.push_back(box_id);
	return getBody(box_id);
}

PhysicsBody PhysicsWorld::addMesh(entt::entity entity, std::vector<glm::vec3> &vertices, std::vector<uint32_t> &indices, glm::vec3 position, MotionType motionType)
{
	auto &body_interface = physicsSystem->GetBodyInterface();

	VertexList inVertices;
	inVertices.reserve(vertices.size());
	IndexedTriangleList inTriangles;
	inTriangles.reserve(indices.size());

	for (int i = 0; i < vertices.size(); i++)
	{
		inVertices.emplace_back(vertices[i].x, vertices[i].y, vertices[i].z);
	}

	for (int i = 0; i < indices.size(); i += 3)
	{
		inTriangles.emplace_back(indices[i], indices[i + 1], indices[i + 2]);
	}

	BodyCreationSettings mesh_settings;
	RVec3 pos(position.x, position.y, position.z);
	auto meshShapeSettings = new MeshShapeSettings(inVertices, inTriangles);

	if (motionType == MotionType::Kinematic)
	{
		mesh_settings = BodyCreationSettings(meshShapeSettings, pos, Quat::sIdentity(), EMotionType::Kinematic, Layers::MOVING);
		mesh_settings.mOverrideMassProperties = EOverrideMassProperties::MassAndInertiaProvided;
		mesh_settings.mMassPropertiesOverride = MassProperties();
		mesh_settings.mMassPropertiesOverride.mMass = 1.0f;
	}
	else // motionType == Dynamic for meshes is not supported by Jolt
	{
		mesh_settings = BodyCreationSettings(meshShapeSettings, pos, Quat::sIdentity(), EMotionType::Static, Layers::NON_MOVING);
	}

	auto mesh_id = body_interface.CreateAndAddBody(mesh_settings, EActivation::Activate);

	body_interface.SetLinearVelocity(mesh_id, Vec3(0.01f, 0.0f, 1.0f));

	if (mesh_id.IsInvalid())
	{
		handleInvalidId("Failed to create mesh", mesh_id);
	}

	setUserData(mesh_id, entity);
	bodies.push_back(mesh_id);
	return getBody(mesh_id);
}

std::vector<std::pair<glm::vec3, glm::vec3>> PhysicsWorld::debugDraw()
{
	BodyManager::DrawSettings settings;
	physicsSystem->DrawBodies(settings, recorder);
	recorder->EndFrame();
	return {};
}

void PhysicsWorld::removeBody(IDType bodyID)
{
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();
	body_interface.RemoveBody(bodyID);
	body_interface.DestroyBody(bodyID);
}

PhysicsBody PhysicsWorld::getBody(IDType bodyID)
{
	return {
		bodyID,
		getMotionType(bodyID),
		getBodyPosition(bodyID),
		getBodyRotation(bodyID),
		getBodyScale(bodyID)};
}

void PhysicsWorld::getBody(IDType bodyID, PhysicsBody &body)
{
	body.bodyID = bodyID;
	body.position = getBodyPosition(bodyID);
	body.rotation = getBodyRotation(bodyID);
	body.scale = getBodyScale(bodyID);
	body.velocity = getBodyVelocity(bodyID);
}

void PhysicsWorld::updateBody(IDType bodyID, PhysicsBody body)
{
	setBodyPosition(bodyID, body.position);
	setBodyRotation(bodyID, body.rotation);
	setBodyScale(bodyID, body.scale);
	setBodyVelocity(bodyID, body.velocity);
}

MotionType PhysicsWorld::getMotionType(IDType bodyID)
{
	auto &body_interface = physicsSystem->GetBodyInterface();
	auto motion_type = body_interface.GetMotionType(bodyID);
	switch (motion_type)
	{
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

glm::vec3 PhysicsWorld::getBodyPosition(IDType bodyID)
{
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();
	RVec3 position = body_interface.GetCenterOfMassPosition(bodyID);
	return glm::vec3(position.GetX(), position.GetY(), position.GetZ());
}

glm::vec4 PhysicsWorld::getBodyRotation(IDType bodyID)
{
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();
	Quat rotation = body_interface.GetRotation(bodyID);
	return glm::vec4(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());
}

glm::vec3 PhysicsWorld::getBodyScale(IDType bodyID)
{
	return glm::vec3(1.0f); // TODO: Implement (Does not appear that jolt has easy scaling)
}

glm::vec3 PhysicsWorld::getBodyVelocity(IDType bodyID)
{
	BodyInterface &body_interface = physicsSystem->GetBodyInterface();
	Vec3 velocity = body_interface.GetLinearVelocity(bodyID);
	return glm::vec3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
}

void PhysicsWorld::setBodyPosition(IDType bodyID, glm::vec3 position)
{
	auto &body_interface = physicsSystem->GetBodyInterface();
	// TODO: Maybe bool for if it should activate?
	body_interface.SetPosition(bodyID, RVec3(position.x, position.y, position.z), EActivation::Activate);
}

void PhysicsWorld::setBodyRotation(IDType bodyID, glm::vec4 rotation)
{
	auto &body_interface = physicsSystem->GetBodyInterface();
	body_interface.SetRotation(bodyID, Quat(rotation.w, rotation.x, rotation.y, rotation.z), EActivation::Activate);
}

void PhysicsWorld::setBodyScale(IDType bodyID, glm::vec3 scale)
{
	// TODO: Implement (Does not appear that jolt has easy scaling)
}

void PhysicsWorld::setBodyVelocity(IDType bodyID, glm::vec3 velocity)
{
	auto &body_interface = physicsSystem->GetBodyInterface();
	body_interface.SetLinearVelocity(bodyID, Vec3(velocity.x, velocity.y, velocity.z));
}

void PhysicsWorld::setUserData(IDType bodyID, entt::entity entity)
{
	auto &body_interface = physicsSystem->GetBodyLockInterface();
	JPH::BodyLockWrite lock(body_interface, bodyID);
	lock.GetBody().SetUserData(static_cast<uint64_t>(entity));
}

entt::entity PhysicsWorld::getUserData(IDType bodyID)
{
	auto &body_interface = physicsSystem->GetBodyInterface();
	return static_cast<entt::entity>(body_interface.GetUserData(bodyID));
}

void PhysicsWorld::handleInvalidId(std::string error, IDType bodyID)
{
}

void PhysicsWorld::update(float dt, entt::registry& registry)
{
	accumulator += dt;
	while (accumulator >= cDeltaTime)
	{
		accumulator -= cDeltaTime;
		BodyInterface &body_interface = physicsSystem->GetBodyInterface();

		for (auto id : bodies)
		{
			if (body_interface.IsActive(id) == false)
				continue;
			// Output current position and velocity of the sphere
			RVec3 position = body_interface.GetCenterOfMassPosition(id);
			Vec3 velocity = body_interface.GetLinearVelocity(id);
			cout << "ID " << id.GetIndex() << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;
		}

		// If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
		const int cCollisionSteps = 1;

		// If you want more accurate step results you can do multiple sub steps within a collision step. Usually you would set this to 1.
		const int cIntegrationSubSteps = 1;

		// Step the world
		physicsSystem->Update(cDeltaTime, cCollisionSteps, cIntegrationSubSteps, tempAllocator.get(), jobSystem.get());

		// Update the bodies
		registry.view<PhysicsBody>().each([this](entt::entity entity, PhysicsBody& body) {
			getBody(body.bodyID, body);
		});
	}
}