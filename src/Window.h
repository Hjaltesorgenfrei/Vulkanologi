#pragma once
#include <utility>
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
	[[nodiscard]] GLFWwindow* getGLFWwindow() const;
	[[nodiscard]] bool windowShouldClose() const;
	void fullscreenWindow();
	[[nodiscard]] bool isFullScreen();
};
