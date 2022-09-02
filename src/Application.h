#pragma once
#include "Renderer.h"
#include "RenderData.h"

class App {
public:
	void run();

private:
	std::shared_ptr<WindowWrapper> window;
	std::shared_ptr<RenderData> model;
	std::unique_ptr<Renderer> renderer;
    bool mouseCaptured = false;

	void mainLoop();
    void processPressedKeys(double delta);
    void setupCallBacks();

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void cursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn);
    static void cursorEnterCallback(GLFWwindow *window, int enter);
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
};
