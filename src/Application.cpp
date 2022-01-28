#define GLFW_INCLUDE_VULKAN
#include <algorithm>
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <fstream>

#include "Renderer.h"
#include "Window.h"
#include "Application.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;



void App::run() {
	window =
		std::make_shared<WindowWrapper>(WIDTH, HEIGHT, "Vulkan Tutorial");
	model = std::make_shared<Model>();
	renderer = std::make_unique<Renderer>(window, model);
    setupCallBacks();
    mainLoop();
}

void App::setupCallBacks() {
    glfwSetWindowUserPointer(window->getGLFWwindow(), this);
    glfwSetFramebufferSizeCallback(window->getGLFWwindow(), framebufferResizeCallback);
    glfwSetCursorPosCallback(window->getGLFWwindow(), cursorPosCallback);
    glfwSetCursorEnterCallback(window->getGLFWwindow(), cursorEnterCallback);
    glfwSetMouseButtonCallback(window->getGLFWwindow(), mouseButtonCallback);
    glfwSetKeyCallback(window->getGLFWwindow(), keyCallback);
}

void App::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->renderer->framebufferResized = true;
}

void App::cursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn) {
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app->mouseCaptured) {
        auto xPos = static_cast<float>(xPosIn);
        auto yPos = static_cast<float>(yPosIn);
        app->model->newCursorPos(xPos, yPos);
    }
}

void App::cursorEnterCallback(GLFWwindow* window, int enter) {
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->model->resetCursorPos();
}

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        if (!app->mouseCaptured) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            app->model->resetCursorPos();
            app->mouseCaptured = true;
        }
    }
}

void App::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        if (app->mouseCaptured) {
            app->mouseCaptured = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else {
            glfwSetWindowShouldClose(window, true);
        }
    }
    if(key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        app->window->fullscreenWindow();
    }
}

void App::mainLoop() {
	while (!window->windowShouldClose()) {
		glfwPollEvents();
        processPressedKeys(window->getGLFWwindow());
		renderer->drawFrame();
	}
}

void App::processPressedKeys(GLFWwindow* window) {
    const float cameraSpeed = 0.005f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        model->moveCameraForward(cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        model->moveCameraBackward(cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        model->moveCameraLeft(cameraSpeed);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        model->moveCameraRight(cameraSpeed);
}

int main() {
	App app;
	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
