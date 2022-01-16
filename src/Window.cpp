#include "Window.h"

WindowWrapper::WindowWrapper(const int width, const int height, const char* title) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

WindowWrapper::~WindowWrapper() {
	glfwDestroyWindow(window);

	glfwTerminate();
}

std::pair<int, int> WindowWrapper::getFramebufferSize() const {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return {width, height};
}

GLFWwindow* WindowWrapper::getGLFWwindow() const {
	return window;
}

bool WindowWrapper::windowShouldClose() const {
	return glfwWindowShouldClose(window);
}
