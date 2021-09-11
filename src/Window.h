#pragma once
#include <utility>
#include <GLFW/glfw3.h>

class App;

class WindowWrapper {
private:
	GLFWwindow* window;
	

public:	
	WindowWrapper(int width, int height, const char* title, App* app);
	~WindowWrapper();
	WindowWrapper& operator=(const WindowWrapper&) = delete;
	WindowWrapper(const WindowWrapper&) = delete;
	
	[[nodiscard]] std::pair<int, int> getFramebufferSize() const;
	[[nodiscard]] GLFWwindow* getGLFWwindow() const;


	[[nodiscard]] bool windowShouldClose() const;
};
