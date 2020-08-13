#pragma once
#include <GLFW/glfw3.h>

class Window {
private:
	GLFWwindow* window;

public:
	Window(int width, int height, const char* title);
	~Window();
	Window& operator=(const Window&) = delete;
	Window(const Window&) = delete;

	GLFWwindow* getGLFWwindow();
	
	bool windowShouldClose() const;
};
