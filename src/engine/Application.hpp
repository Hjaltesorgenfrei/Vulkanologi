#pragma once
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#include <entt/entt.hpp>
#include <unordered_map>

#include "Components.hpp"
#include "DependentSystem.hpp"
#include "NetworkServerSystem.hpp"
#include "Physics.hpp"
#include "Renderer.hpp"
#include "SystemGraph.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class App {
public:
	App();

	void run();

	App inline static* instance = nullptr;

private:
	std::shared_ptr<WindowWrapper> window =
		std::make_shared<WindowWrapper>(WIDTH, HEIGHT, "Rotte Simulator: Live your best life as a rat");
	std::unique_ptr<Renderer> renderer;
	std::shared_ptr<BehDevice> device;
	std::unique_ptr<NetworkServerSystem> networkServerSystem;
	entt::registry registry;
	std::unordered_set<entt::entity> entities;
	SystemGraph systemGraph;
	std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
	std::vector<std::string> swiperNames;
	Material carMaterial;
	Material noMaterial;

	std::unique_ptr<PhysicsWorld> physicsWorld;

	bool showDebugInfo = false;
	bool updateWindowSize = false;
	double cursorDeltaX = 0.0, cursorDeltaY = 0.0;
	double cursorPosX = 0.0, cursorPosY = 0.0;
	bool cursorShouldReset = true;
	bool cursorHidden = false;
	ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

	void mainLoop();
	void setupCallBacks();
	bool drawImGuizmo(glm::mat4* matrix, glm::mat4* deltaMatrix);
	void setupWorld();
	void bezierTesting();
	void createSpawnPoints(int numberOfSpawns);
	void setupControllerPlayers();
	std::vector<Path> drawNormals(std::shared_ptr<RenderObject> object);
	Camera& getCamera();

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
	static void cursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn);
	static void cursorEnterCallback(GLFWwindow* window, int enter);
	static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void joystickCallback(int joystickId, int event);

	template <typename T>
	entt::entity addPlayer(T input);

	entt::entity addSwiper(Axis direction, float speed, int swiper);
	void loadSwipers();

	bool shiftPressed = false;

	int drawFrame(float delta);
	void drawFrameDebugInfo(float delta, FrameInfo& frameInfo);
	// void drawRigidBodyDebugInfo(btRigidBody* rigidBody);
	void drawDebugForSelectedEntity(entt::entity selectedEntity, FrameInfo& frameInfo);
};