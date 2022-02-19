#include "Window.h"
#include <iostream>

void glfwErrorCallback(int code, const char* description)
{
	std::cerr << "GLFW Error " << code << ": " << description << std::endl;
}

WindowWrapper::WindowWrapper(const int width, const int height, const char* title) {
	glfwInit();
	glfwSetErrorCallback(glfwErrorCallback);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
}

WindowWrapper::~WindowWrapper() {
	glfwDestroyWindow(window);

	glfwTerminate();
}

std::pair<int, int> WindowWrapper::getFramebufferSize() const {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	return {width, height};
}

GLFWwindow* WindowWrapper::getGLFWwindow() const {
	return window;
}

bool WindowWrapper::windowShouldClose() const {
	return glfwWindowShouldClose(window);
}

void WindowWrapper::fullscreenWindow() {
	auto monitor = getCurrentMonitor(window);
	const auto mode = glfwGetVideoMode(monitor);
	if (isFullScreen()) {
		glfwSetWindowMonitor(window, NULL, 30, 30, 1024, 640, mode->refreshRate);
	}
	else {
		glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
	}
}

bool WindowWrapper::isFullScreen() {
	return glfwGetWindowMonitor(window) != nullptr;
}

static int mini(int x, int y)
{
    return x < y ? x : y;
}

static int maxi(int x, int y)
{
    return x > y ? x : y;
}

GLFWmonitor* WindowWrapper::getCurrentMonitor(GLFWwindow *window)
{
    int nMonitors, i;
    int wx, wy, ww, wh;
    int mx, my, mw, mh;
    int overlap, bestoverlap;
    GLFWmonitor *bestmonitor;
    GLFWmonitor **monitors;
    const GLFWvidmode *mode;

    bestoverlap = 0;
    bestmonitor = NULL;

    glfwGetWindowPos(window, &wx, &wy);
    glfwGetWindowSize(window, &ww, &wh);
    monitors = glfwGetMonitors(&nMonitors);

    for (i = 0; i < nMonitors; i++) {
        mode = glfwGetVideoMode(monitors[i]);
        glfwGetMonitorPos(monitors[i], &mx, &my);
        mw = mode->width;
        mh = mode->height;

        overlap =
            maxi(0, mini(wx + ww, mx + mw) - maxi(wx, mx)) *
            maxi(0, mini(wy + wh, my + mh) - maxi(wy, my));

        if (bestoverlap < overlap) {
            bestoverlap = overlap;
            bestmonitor = monitors[i];
        }
    }

    return bestmonitor;
}