#pragma once
#include <utility>
#include <GLFW/glfw3.h>

class App;

class Window {
private:
	GLFWwindow* window;
	

public:	
	Window(int width, int height, const char* title, App* app);
	~Window();
	Window& operator=(const Window&) = delete;
	Window(const Window&) = delete;
	
	[[nodiscard]] std::pair<int, int> getFramebufferSize() const;
	[[nodiscard]] GLFWwindow* getGLFWwindow() const;


	[[nodiscard]] bool windowShouldClose() const;
};
