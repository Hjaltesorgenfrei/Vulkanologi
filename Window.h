#pragma once
#define GLFW_INCLUDE_VULKAN
#include <algorithm>
#include <GLFW/glfw3.h>


#include <iostream>
#include <functional>
#include <stdexcept>
#include <cstdlib>
#include <fstream>

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
