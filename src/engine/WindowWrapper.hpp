#ifndef WINDOWWRAPPER_H
#define WINDOWWRAPPER_H
#pragma once

#include <utility>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class WindowWrapper {
private:
	GLFWwindow* window;
	GLFWmonitor* getCurrentMonitor(GLFWwindow *window);

public:	
	WindowWrapper(int width, int height, const char* title);
	~WindowWrapper();
	WindowWrapper& operator=(const WindowWrapper&) = delete;
	WindowWrapper(const WindowWrapper&) = delete;
	
	[[nodiscard]] std::pair<int, int> getFramebufferSize() const;
	[[nodiscard]] int getWidth() const;
	[[nodiscard]] int getHeight() const;
	[[nodiscard]] GLFWwindow* getGLFWwindow() const;
	[[nodiscard]] bool windowShouldClose() const;
	void fullscreenWindow();
	[[nodiscard]] bool isFullScreen();
	void setTitle(const char* title);
};

#endif