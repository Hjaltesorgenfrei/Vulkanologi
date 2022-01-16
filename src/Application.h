#pragma once
#include "Renderer.h"
#include "Model.h"

class App {
public:
	void run();

private:
	std::shared_ptr<WindowWrapper> window;
	std::shared_ptr<Model> model;
	std::unique_ptr<Renderer> renderer;

	void mainLoop();
    void processInput(GLFWwindow *window);
    void setupCallBacks();

    static void framebufferResizeCallback(GLFWwindow *window, int width, int height);
    static void cursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn);
    static void cursorEnterCallback(GLFWwindow *window, int enter);
};
