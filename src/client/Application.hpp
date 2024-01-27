#pragma once
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <ImGuizmo.h>

#include <entt/entt.hpp>
#include <unordered_map>

#include "Components.hpp"
#include "DependentSystem.hpp"
#include "INetworkSystem.hpp"
#include "Physics.hpp"
#include "Renderer.hpp"
#include "SystemGraph.hpp"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char* messages[11] = {"Rotte Simulator: Life is like a rat",
							"Rotte Simmelator: Rotte is love, Rotte is life",
							"Simulator Rotte: I wish i were a rat",
							"Rotte Simulator: Kan rotter danse tango?",
							"Rotte Simulator: Min rotte den danser tango",
							"Rote Simmulator: Nu med flere rotter!!111!1!1!!",
							"Rotte Simultor: Smager en smugle af citron",
							"Rotte Simulator: Prøv minecraft!",
							"Rotte Simulator: Prøv terraria!!",
							"Rotte Simulator: Nu med 0.01% mindre asbest!",
							"Rotte Simulator: Live your best life as a rat"};

class App {
public:
	App();

	void run(int argc, char* argv[]);

	App inline static* instance = nullptr;

private:
	std::shared_ptr<WindowWrapper> window;
	std::unique_ptr<Renderer> renderer;
	std::shared_ptr<BehDevice> device;
	std::unique_ptr<INetworkSystem> networkSystem;
	entt::registry registry;
	std::unordered_set<entt::entity> entities;
	SystemGraph systemGraph;
	std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
	std::vector<std::string> swiperNames;
	Material carMaterial;
	Material noMaterial;
	Material memeMaterial;

	std::unique_ptr<PhysicsWorld> physicsWorld;

	bool showDebugInfo = false;
	bool updateWindowSize = false;
	double cursorDeltaX = 0.0, cursorDeltaY = 0.0;
	double cursorPosX = 0.0, cursorPosY = 0.0;
	bool cursorShouldReset = true;
	bool cursorHidden = false;
	glm::vec3 blackHole = glm::vec3(0.f, 5.f, 0.f);
	float blackHolePower = 2500000.f;
	ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;

	void mainLoop();
	void setupCallBacks();
	bool drawImGuizmo(glm::mat4* matrix, glm::mat4* deltaMatrix);
	void setupWorld();
	void spawnArena();
	void spawnRandomCrap();
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

	void addCubes(int layers, float x, float z);

	template <typename T>
	entt::entity addCubePlayer(T input);

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
