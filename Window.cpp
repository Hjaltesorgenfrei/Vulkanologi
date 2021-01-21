#include "Window.h"

Window::Window(const int width, const int height, const char* title) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

Window::~Window() {
	glfwDestroyWindow(window);

	glfwTerminate();
}

std::pair<int, int> Window::getFramebufferSize() const {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return std::pair<int, int>(width, height);
}

GLFWwindow* Window::getGLFWwindow() const {
	return window;
}

bool Window::windowShouldClose() const {
	return glfwWindowShouldClose(window);
}
