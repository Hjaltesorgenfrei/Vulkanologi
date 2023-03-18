#pragma once
#include "Renderer.h"

#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"
#include "ImGuizmo.h"
#include "Physics.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class App {
public:
    App();

    void run();

private:
    std::shared_ptr<WindowWrapper> window = std::make_shared<WindowWrapper>(WIDTH, HEIGHT, "Vulkan Tutorial");
	std::unique_ptr<Renderer> renderer;
    std::shared_ptr<BehDevice> device;
    std::vector<std::shared_ptr<RenderObject>> objects;

    std::unique_ptr<PhysicsWorld> physicsWorld;

    BehCamera camera{};
    bool showDebugInfo = true;
    bool updateWindowSize = false;
    bool cursorHidden = false;
    ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

	void mainLoop();
    void processPressedKeys(float delta);
    void setupCallBacks();
    bool drawImGuizmo(glm::mat4* matrix);
    std::vector<Path> drawNormals(std::shared_ptr<RenderObject> object);

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void cursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn);
    static void cursorEnterCallback(GLFWwindow *window, int enter);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    bool shiftPressed = false;

    int drawFrame(float delta);

};
