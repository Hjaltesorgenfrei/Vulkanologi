#pragma once
#include <utility>
#include <GLFW/glfw3.h>

class Window {
private:
	GLFWwindow* window;

public:
	Window(int width, int height, const char* title);
	~Window();
	Window& operator=(const Window&) = delete;
	Window(const Window&) = delete;

	std::pair<int, int> getFramebufferSize();
	GLFWwindow* getGLFWwindow() const;

	
	bool windowShouldClose() const;
};
