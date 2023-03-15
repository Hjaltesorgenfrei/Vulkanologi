#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <chrono>
#include <thread>
// Include bullet
#include <btBulletDynamicsCommon.h>

#include "Renderer.h"
#include "Application.h"
#include "ShapeExperiment.h"

#include "Path.h"
#include "Spline.h"

// dynamics world pointer
btDiscreteDynamicsWorld* dynamicsWorld;

class DebugDrawer : public btIDebugDraw {
public:
    void drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color) override {
        auto path = linePath(glm::vec3(from.x(), from.y(), from.z()), glm::vec3(to.x(), to.y(), to.z()), glm::vec3(color.x(), color.y(), color.z()));
        paths.emplace_back(path);
    }

    void drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color) override {
        auto path = linePath(glm::vec3(PointOnB.x(), PointOnB.y(), PointOnB.z()), glm::vec3(PointOnB.x(), PointOnB.y(), PointOnB.z()) + glm::vec3(normalOnB.x(), normalOnB.y(), normalOnB.z()) * distance, glm::vec3(color.x(), color.y(), color.z()));
        paths.emplace_back(path);
    }

    void reportErrorWarning(const char *warningString) override {
        std::cerr << "Warning: " << warningString << std::endl;
    }

    void draw3dText(const btVector3 &location, const char *textString) override {
        std::cerr << "Text: " << textString << std::endl;
    }

    void setDebugMode(int debugMode) override {
        std::cerr << "Debug mode: " << debugMode << std::endl;
    }

    int getDebugMode() const override {
        return DBG_DrawWireframe;
    }

    // clear lines
    void clearLines() override {
        paths.clear();
    }

    std::vector<Path> paths;

};

DebugDrawer * debugDrawer;

void App::run() {
    setupCallBacks(); // We create ImGui in the renderer, so callbacks have to happen before.
    device = std::make_unique<BehDevice>(window);
    AssetManager manager(device);
	renderer = std::make_unique<Renderer>(window, device, manager);
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
    app->updateWindowSize = true;
}

void App::cursorPosCallback(GLFWwindow* window, double xPosIn, double yPosIn) {
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (app->cursorHidden) {
        auto xPos = static_cast<float>(xPosIn);
        auto yPos = static_cast<float>(yPosIn);
        app->camera.newCursorPos(xPos, yPos);
    }
}

void App::cursorEnterCallback(GLFWwindow* window, int enter) {
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    app->camera.resetCursorPos();
}

void rayTest(btDiscreteDynamicsWorld* dynamicsWorld, const btVector3& rayFromWorld, const btVector3& rayToWorld) {
    btCollisionWorld::ClosestRayResultCallback rayCallback(rayFromWorld, rayToWorld);
    dynamicsWorld->rayTest(rayFromWorld, rayToWorld, rayCallback);
    if (rayCallback.hasHit()) {
        btVector3 hitPoint = rayCallback.m_hitPointWorld;
        btVector3 hitNormal = rayCallback.m_hitNormalWorld;
        const btRigidBody* body = btRigidBody::upcast(rayCallback.m_collisionObject);
        if (body) {
            body->setActivationState(ACTIVE_TAG);
        }
    }
}

std::vector<Path> rays;

void App::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
        return;
    }

    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && !app->shiftPressed) {
        if (!app->cursorHidden) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            app->camera.resetCursorPos();
            app->cursorHidden = true;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS && app->shiftPressed) {
        // raytest with bullet
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        auto pos = app->camera.getCameraPosition();
        auto dir = app->camera.getRayDirection(x, y, app->window->getWidth(), app->window->getHeight());
        btVector3 rayFromWorld(pos.x, pos.y, pos.z);
        const int maxRayLength = 1000;
        btVector3 rayToWorld(pos.x + dir.x * maxRayLength, pos.y + dir.y * maxRayLength, pos.z + dir.z * maxRayLength);
        rayTest(dynamicsWorld, rayFromWorld, rayToWorld);

        glm::vec3 rayFromWorldGlm(rayFromWorld.x(), rayFromWorld.y(), rayFromWorld.z());
        glm::vec3 rayToWorldGlm(rayToWorld.x(), rayToWorld.y(), rayToWorld.z());
        rays.clear();
        rays.push_back(linePath(rayFromWorldGlm, rayToWorldGlm, glm::vec3(1, 0, 0)));

    }
    else if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_RELEASE) {
        if (app->cursorHidden) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            app->cursorHidden = false;
        }
    }
}

void App::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto* const app = static_cast<App*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
    }
    if(key == GLFW_KEY_F11 && action == GLFW_PRESS) {
        app->window->fullscreenWindow();
    }
	if(key == GLFW_KEY_F1 && action == GLFW_PRESS) {
		app->renderer->rendererMode = NORMAL;
	}
	if(key == GLFW_KEY_F2 && action == GLFW_PRESS) {
		app->renderer->rendererMode = WIREFRAME;
	}
    if(key == GLFW_KEY_E && action == GLFW_PRESS) {
        app->showDebugInfo = !app->showDebugInfo;
    }
    if(key == GLFW_KEY_LEFT_SHIFT && action != GLFW_RELEASE) {
        app->shiftPressed = true;
    }
    if(key == GLFW_KEY_LEFT_SHIFT && action == GLFW_RELEASE) {
        app->shiftPressed = false;
    }
    // Set to translate mode on z, rotate on x, scale on c
    if(key == GLFW_KEY_Z && action != GLFW_RELEASE) {
        app->currentGizmoOperation = ImGuizmo::TRANSLATE;
    }
    if(key == GLFW_KEY_X && action != GLFW_RELEASE) {
        app->currentGizmoOperation = ImGuizmo::ROTATE;
    }
    if(key == GLFW_KEY_C && action != GLFW_RELEASE) {
        app->currentGizmoOperation = ImGuizmo::SCALE;
    }
}

float bytesToMegaBytes(uint64_t bytes) {
    return bytes / 1024.0f / 1024.0f;
}

Path circleAroundPoint(glm::vec3 center, float radius, int segments) {
    Path path(true);
    for (int i = 0; i < segments; i++) {
        float angle = (float)i / (float)segments * 2.0f * 3.14159265359f;
        glm::vec3 point = center + glm::vec3(cos(angle) * radius, sin(angle) * radius, 0);
        path.addPoint({point, glm::vec3(1, 1, 1), glm::vec3(0, 1, 0)});
    }
    return path;
}

int App::drawFrame(float delta) {
    // std::lock_guard<std::mutex> lockGuard(rendererMutex);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto memoryUsage = renderer->getMemoryUsage();
    // Make a imgui window to show the frame time

    // Save average of last 10 frames
    static const int frameCount = 30;
    static float frameTimes[frameCount] = {0};
    static int frameTimeIndex = 0;
    frameTimes[frameTimeIndex] = delta;
    frameTimeIndex = (frameTimeIndex + 1) % frameCount;
    float averageFrameTime = 0;
    for (int i = 0; i < frameCount; i++) {
        averageFrameTime += frameTimes[i];
    }
    averageFrameTime /= frameCount;

    static int resolution = 10;
    static int segments = 50;
    static float along = 0.f;
    
    along += 0.01f;
    if (along > 2.f) {
        along = 0.f;
    }

    //ImGui::ShowDemoWindow();
    FrameInfo frameInfo{};
    frameInfo.objects = objects;
    frameInfo.camera = camera;
    frameInfo.deltaTime = delta;

    static ControlPoint start;
    static ControlPoint end;

    if (showDebugInfo) {
        ImGui::Begin("Debug Info");
        ImGui::Text("Frame Time: %f", averageFrameTime);
        ImGui::Text("FPS: %f", 1000.0 / averageFrameTime);
        ImGui::Text("Memory Usage: %.1fmb", bytesToMegaBytes(memoryUsage));
        ImGui::SliderInt("Resolution", &resolution, 1, 10);
        ImGui::SliderInt("Segments", &segments, 2, 50);
        ImGui::End();

        drawImGuizmo(&start.transform);

        auto path = cubicPath(
            start,
            end,
            segments, 
            resolution, glm::vec3{1, 0, 0});
        frameInfo.paths.emplace_back(path);
        for (const auto point : path.getPoints()) {
            frameInfo.paths.emplace_back(linePath(point.position, point.position + point.normal * 0.5f, {0, 0, 1}));
        }
        for (const auto frame : generateRMFrames(start.point(), start.forwardWorld(), end.backwardWorld(), end.point().position, segments, resolution)) {
            auto start = frame.o;
            auto rightVector = glm::normalize(frame.r);
            auto end = frame.o + frame.t * 0.25f;
            auto right = frame.o + frame.t * 0.23f - rightVector * 0.01f;
            auto left = frame.o + frame.t * 0.23f + rightVector * 0.01f;
            frameInfo.paths.emplace_back(linePath(start, end, {0, 1, 0}));
            // Make a arrow at the end
            frameInfo.paths.emplace_back(linePath(end, right, {0, 1, 0}));
            frameInfo.paths.emplace_back(linePath(end, left, {0, 1, 0}));
            frameInfo.paths.emplace_back(linePath(right, left, {0, 1, 0}));
        }

        dynamicsWorld->debugDrawWorld();
        auto physicsPaths = debugDrawer->paths;
        frameInfo.paths.insert(frameInfo.paths.end(), physicsPaths.begin(), physicsPaths.end());

        frameInfo.paths.insert(frameInfo.paths.end(), rays.begin(), rays.end());
    }

    if (!objects.empty()) {
        auto lastModel = objects.back(); // Just a testing statement
        if (along > 1.f)
            lastModel->transformMatrix.model = moveAlongCubicPath(end, start, segments, resolution, along - 1.f);
        else
            lastModel->transformMatrix.model = moveAlongCubicPath(start, end, segments, resolution, along);
    }

    auto result = renderer->drawFrame(frameInfo);
    ImGui::EndFrame();
    return result;
}

void App::mainLoop() {
    // make bullet3 physics world
    auto collisionConfiguration = new btDefaultCollisionConfiguration();
    auto dispatcher = new btCollisionDispatcher(collisionConfiguration);
    auto overlappingPairCache = new btDbvtBroadphase();
    auto solver = new btSequentialImpulseConstraintSolver;
    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -10, 0));

    // Create a ground plane
    auto groundShape = new btStaticPlaneShape(btVector3(0, 1, 0), 1);
    auto groundTransform = btTransform();
    groundTransform.setIdentity();
    groundTransform.setOrigin(btVector3(0, -1, 0));
    auto groundMass = 0.f;
    auto groundLocalInertia = btVector3(0, 0, 0);
    auto groundMotionState = new btDefaultMotionState(groundTransform);
    auto groundRigidBodyCI = btRigidBody::btRigidBodyConstructionInfo(groundMass, groundMotionState, groundShape, groundLocalInertia);
    auto groundRigidBody = new btRigidBody(groundRigidBodyCI);
    dynamicsWorld->addRigidBody(groundRigidBody);

    // add 5 dynamic boxes
    auto colShape = new btBoxShape(btVector3(1, 1, 1));
    auto startTransform = btTransform();
    startTransform.setIdentity();
    auto mass = 1.f;
    auto localInertia = btVector3(0, 0, 0);
    colShape->calculateLocalInertia(mass, localInertia);
    for (int i = 0; i < 5; i++) {
        startTransform.setOrigin(btVector3(2 * i, 10, 0));
        auto myMotionState = new btDefaultMotionState(startTransform);
        auto rbInfo = btRigidBody::btRigidBodyConstructionInfo(mass, myMotionState, colShape, localInertia);
        auto body = new btRigidBody(rbInfo);
        body->setUserIndex(i);
        dynamicsWorld->addRigidBody(body);
    }

    // Add debug drawing
    debugDrawer = new DebugDrawer();
    dynamicsWorld->setDebugDrawer(debugDrawer);

    auto timeStart = std::chrono::high_resolution_clock::now();
    // objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/lost_empire.obj"), Material{}));
    objects.push_back(std::make_shared<RenderObject>(createCube(glm::vec3{}), Material{}));
    objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/rat.obj"), Material{}));
    renderer->uploadMeshes(objects);

	while (!window->windowShouldClose()) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> delta = now - timeStart;
        timeStart = now;
		glfwPollEvents();
        processPressedKeys(delta.count());

        debugDrawer->clearLines();
        dynamicsWorld->stepSimulation(delta.count() / 1000.f, 10);
        
        if (updateWindowSize) {
            renderer->recreateSwapchain();
            updateWindowSize = false;
        }
        auto result = drawFrame(delta.count());
        if (result == 1) {
            renderer->recreateSwapchain();
        }
	}

    delete debugDrawer;
    delete dynamicsWorld;
    delete solver;
    delete overlappingPairCache;
    delete dispatcher;
    delete collisionConfiguration;
}

bool App::drawImGuizmo(glm::mat4* matrix) {
    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(true);
    auto [width, height] = window->getFramebufferSize();
    auto proj = camera.getCameraProjection(static_cast<float>(width), static_cast<float>(height));
    proj[1][1] *= -1; // ImGuizmo Expects the opposite
    ImGuiIO& io = ImGui::GetIO();
    ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
    return ImGuizmo::Manipulate(&camera.viewMatrix()[0][0], &proj[0][0], currentGizmoOperation, ImGuizmo::LOCAL, &(*matrix)[0][0]);
}

void App::processPressedKeys(float delta) {
    auto glfw_window = window->getGLFWwindow();
    float cameraSpeed = 0.005f * delta;
    if (shiftPressed) {
        cameraSpeed *= 4;
    }
    if (glfwGetKey(glfw_window, GLFW_KEY_W) == GLFW_PRESS)
        camera.moveCameraForward(cameraSpeed);
    if (glfwGetKey(glfw_window, GLFW_KEY_S) == GLFW_PRESS)
        camera.moveCameraBackward(cameraSpeed);
    if (glfwGetKey(glfw_window, GLFW_KEY_A) == GLFW_PRESS)
        camera.moveCameraLeft(cameraSpeed);
    if (glfwGetKey(glfw_window, GLFW_KEY_D) == GLFW_PRESS)
        camera.moveCameraRight(cameraSpeed);
}

App::App() = default;

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
