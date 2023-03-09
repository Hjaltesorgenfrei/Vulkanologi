#pragma once
#include "Renderer.h"

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
    BehCamera camera{};
    bool showImguizmo = true;
    bool updateWindowSize = false;

	void mainLoop();
    void processPressedKeys(float delta);
    void setupCallBacks();
    void drawImGuizmo(glm::mat4* matrix);

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void cursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn);
    static void cursorEnterCallback(GLFWwindow *window, int enter);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    bool shiftPressed = false;

    void drawLoop();

    int drawFrame(float delta);

};
