#pragma once 
#include "../DependentSystem.hpp"
#include "../Components.hpp"

struct SensorTransformSystem : Reads<Sensor>::Writes<Transform>::Named<"SensorTransformSystem"> {
    virtual void run(float delta, Sensor sensor, Transform& transform) const override;
};

struct RigidBodySystem : Reads<RigidBody>::Writes<Transform>::Named<"RigidBodySystem"> {
    virtual void run(float delta, RigidBody rigidBody, Transform& transform) const override;
};

struct CarTransformSystem : Reads<Car>::Writes<Transform>::Named<"CarTransformSystem"> {
    virtual void run(float delta, Car car, Transform& transform) const override;
};

struct TransformControlPointsSystem : Reads<Transform>::Writes<ControlPointPtr>::Named<"TransformControlPointsSystem"> {
    virtual void run(float delta, Transform transform, ControlPointPtr& controlPoint) const override;
};