#define GLFW_INCLUDE_VULKAN
#include <algorithm>
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <fstream>

#include "Renderer.h"
#include "Window.h"


class HelloTriangleApplication {
public:
	void run() {
		window = std::make_unique<Window>(WIDTH, HEIGHT, "Vulkan Tutorial");
		renderer = std::make_unique<Renderer>(window);
		mainLoop();
	}

private:
	std::unique_ptr<Window> window;
	std::unique_ptr<Renderer> renderer;


	void mainLoop() {
		while (!window->windowShouldClose()) {
			glfwPollEvents();
			renderer->drawFrame();
		}
	}


};

int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
