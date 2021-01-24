#pragma once
#include "Renderer.h"

class App {
public:
	void run();
	void windowResized();

private:
	std::shared_ptr<Window> window;
	std::unique_ptr<Renderer> renderer;

	void framebufferResizeCallback(GLFWwindow* window, int width, int height);

	void mainLoop();


};
