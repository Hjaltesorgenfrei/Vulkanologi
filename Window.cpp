#include "Window.h"

Window::Window(int width, int height, const char* title) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

Window::~Window() {
	glfwDestroyWindow(window);

	glfwTerminate();
}

GLFWwindow* Window::getGLFWwindow() {
	return window;
}

bool Window::windowShouldClose() const {
	return glfwWindowShouldClose(window);
}
