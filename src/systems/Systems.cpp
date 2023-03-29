#include "Systems.hpp"
#include "CarSystem.hpp"
#include "ControllerSystem.hpp"
#include "TransformSystems.hpp"
#include "SwiperSystem.hpp"

void setupSystems(SystemGraph& systemGraph)
{
    systemGraph.addSystem<CarKeyboardSystem>();
    systemGraph.addSystem<CarJoystickSystem>();
    systemGraph.addSystem<ControllerSystem>();
    systemGraph.addSystem<CarSystem>();
    systemGraph.addSystem<SensorTransformSystem>();
    systemGraph.addSystem<RigidBodySystem>();
    systemGraph.addSystem<CarTransformSystem>();
    systemGraph.addSystem<TransformControlPointsSystem>();
    systemGraph.addSystem<SwiperSystem>();
}