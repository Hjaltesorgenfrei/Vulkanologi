#include "Window.h"

#include "Application.h"


Window::Window(const int width, const int height, const char* title, App* app) {
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	glfwSetWindowUserPointer(window, app);
	glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
}

Window::~Window() {
	glfwDestroyWindow(window);

	glfwTerminate();
}


void Window::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
	app->windowResized();
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
