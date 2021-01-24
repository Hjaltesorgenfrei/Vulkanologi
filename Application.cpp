#define GLFW_INCLUDE_VULKAN
#include <algorithm>
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <fstream>

#include "Renderer.h"
#include "Window.h"
#include "Application.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;



void App::run() {
	window =
		std::make_shared<Window>(WIDTH, HEIGHT, "Vulkan Tutorial", this);
	renderer = std::make_unique<Renderer>(window);
	mainLoop();
}

void App::windowResized() {
	renderer->framebufferResized = true;
	renderer->drawFrame();
}

void App::framebufferResizeCallback(GLFWwindow* window, int width, int height) {

}

void App::mainLoop() {
	while (!window->windowShouldClose()) {
		glfwPollEvents();
		renderer->drawFrame();
	}
}

int main() {
	App app;
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
