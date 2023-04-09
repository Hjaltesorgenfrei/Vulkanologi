#include "Systems.hpp"
#include "CarSystem.hpp"
#include "ControllerSystem.hpp"
#include "TransformSystems.hpp"

void setupSystems(SystemGraph& systemGraph)
{
    systemGraph.addSystem<CarKeyboardSystem>();
    systemGraph.addSystem<CarJoystickSystem>();
    systemGraph.addSystem<ControllerSystem>();
    systemGraph.addSystem<CarSystem>();
    systemGraph.addSystem<RigidBodySystem>();
    systemGraph.addSystem<TransformControlPointsSystem>();
    systemGraph.addSystem<CarCameraSystem>();
}