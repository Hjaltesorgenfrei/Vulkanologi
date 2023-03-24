#pragma once
#include <entt/entt.hpp>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <ImGuizmo.h>
#include <unordered_map>

#include "Renderer.hpp"
#include "Physics.hpp"
#include "Components.hpp"
#include "DependentSystem.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class App {
public:
    App();

    void run();

    App inline static* instance = nullptr;

private:
    std::shared_ptr<WindowWrapper> window = std::make_shared<WindowWrapper>(WIDTH, HEIGHT, "Vulkan Tutorial");
	std::unique_ptr<Renderer> renderer;
    std::shared_ptr<BehDevice> device;
    entt::entity selectedEntity = entt::null;
    entt::registry registry;
    std::vector<entt::entity> entities;
    entt::entity keyboardPlayer; // Index is important
    std::vector<std::shared_ptr<System>> systems;

    std::unique_ptr<PhysicsWorld> physicsWorld;

    BehCamera camera{};
    bool showDebugInfo = true;
    bool updateWindowSize = false;
    bool cursorHidden = false;
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

	void mainLoop();
    void updateSystems(float delta);
    void processPressedKeys(float delta);
    void setupCallBacks();
    bool drawImGuizmo(glm::mat4* matrix);
    void setupWorld();
    void setupSystems();
    void setupControllerPlayers();
    std::vector<Path> drawNormals(std::shared_ptr<RenderObject> object);

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void cursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn);
    static void cursorEnterCallback(GLFWwindow *window, int enter);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);
    static void joystickCallback(int joystickId, int event);

    void createJoystickPlayer(int joystickId);

    void onRigidBodyDestroyed(entt::registry &registry, entt::entity entity);
    void onSensorDestroyed(entt::registry &registry, entt::entity entity);
    void onCarDestroyed(entt::registry &registry, entt::entity entity);

    bool shiftPressed = false;

    int drawFrame(float delta);
    void drawFrameDebugInfo(float delta, FrameInfo& frameInfo);
    void drawRigidBodyDebugInfo(btRigidBody* rigidBody);
};