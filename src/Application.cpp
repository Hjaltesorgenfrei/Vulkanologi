#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <entt/entt.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Renderer.h"
#include "Application.h"
#include "ShapeExperiment.h"

#include "Path.h"
#include "Spline.h"

entt::registry registry;

btRigidBody * testBody;

void App::run() {
    setupCallBacks(); // We create ImGui in the renderer, so callbacks have to happen before.
    device = std::make_unique<BehDevice>(window);
    AssetManager manager(device);
	renderer = std::make_unique<Renderer>(window, device, manager);
    physicsWorld = std::make_unique<PhysicsWorld>();
  
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
        auto dir = app->camera.getRayDirection(static_cast<float>(x), static_cast<float>(y), static_cast<float>(app->window->getWidth()), static_cast<float>(app->window->getHeight()));
        btVector3 rayFromWorld(pos.x, pos.y, pos.z);
        const int maxRayLength = 1000;
        btVector3 rayToWorld(pos.x + dir.x * maxRayLength, pos.y + dir.y * maxRayLength, pos.z + dir.z * maxRayLength);
        app->physicsWorld->rayTest(rayFromWorld, rayToWorld);

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
    
    along += 0.001f * delta;
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

        auto physicsPaths = physicsWorld->getDebugLines();
        frameInfo.paths.insert(frameInfo.paths.end(), physicsPaths.begin(), physicsPaths.end());

        frameInfo.paths.insert(frameInfo.paths.end(), rays.begin(), rays.end());
    }

    if (!objects.empty()) {
        auto lastModel = objects.back(); // Just a testing statement
        // drawImGuizmo(&lastModel->transformMatrix.model);

        auto normalPaths = drawNormals(lastModel);
        frameInfo.paths.insert(frameInfo.paths.end(), normalPaths.begin(), normalPaths.end());
    }

    auto transform = testBody->getWorldTransform();
    auto scale = testBody->getCollisionShape()->getLocalScaling();
    // convert to glm
    glm::mat4 modelMatrix;
    transform.getOpenGLMatrix(glm::value_ptr(modelMatrix));

    if (currentGizmoOperation == ImGuizmo::SCALE) {
        modelMatrix = glm::scale(modelMatrix, glm::vec3(scale.x(), scale.y(), scale.z()));
        auto scaleMatrix = glm::mat4(1.0f);
        ImGuizmo::BeginFrame();
        ImGuizmo::Enable(true);
        auto [width, height] = window->getFramebufferSize();
        auto proj = camera.getCameraProjection(static_cast<float>(width), static_cast<float>(height));
        proj[1][1] *= -1; // ImGuizmo Expects the opposite
        ImGuiIO& io = ImGui::GetIO();
        ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
        if(ImGuizmo::Manipulate(&camera.viewMatrix()[0][0], &proj[0][0], currentGizmoOperation, ImGuizmo::LOCAL, &modelMatrix[0][0], &scaleMatrix[0][0])) {
            // Add the scale to the current scale
            btVector3 newScale = btVector3(scaleMatrix[0][0], scaleMatrix[1][1], scaleMatrix[2][2]);
            newScale *= scale;
            testBody->getCollisionShape()->setLocalScaling(newScale);
        }
    }

    else if (drawImGuizmo(&modelMatrix)) {
        // convert back to bullet
        btTransform newTransform;
        newTransform.setFromOpenGLMatrix(glm::value_ptr(modelMatrix));
        testBody->setWorldTransform(newTransform);
    }

    auto result = renderer->drawFrame(frameInfo);
    ImGui::EndFrame();
    return result;
}

void App::mainLoop() {
    auto timeStart = std::chrono::high_resolution_clock::now();
    // objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/lost_empire.obj"), Material{}));
    objects.push_back(std::make_shared<RenderObject>(createCube(glm::vec3{}), Material{}));
    objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/rat.obj"), Material{}));
    objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/road.obj"), Material{}));
    renderer->uploadMeshes(objects);

    auto colShape = new btBoxShape(btVector3(1, 1, 1));
    auto startTransform = btTransform();
    startTransform.setIdentity();
    auto mass = 1.f;
    auto localInertia = btVector3(0, 0, 0);
    colShape->calculateLocalInertia(mass, localInertia);
    startTransform.setOrigin(btVector3(static_cast<btScalar>(5), 10, 2));
    auto myMotionState = new btDefaultMotionState(startTransform);
    auto rbInfo = btRigidBody::btRigidBodyConstructionInfo(mass, myMotionState, colShape, localInertia);
    testBody = new btRigidBody(rbInfo);
    testBody->setUserIndex(8);
    physicsWorld->addBody(testBody);

	while (!window->windowShouldClose()) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> delta = now - timeStart;
        timeStart = now;
		glfwPollEvents();
        processPressedKeys(delta.count());

        // TODO: Move this to a physics thread
        // TODO: Make this a fixed timestep
        // TODO: Give milliseconds type as argument
        physicsWorld->update(delta.count() / 1000.f);
        
        if (updateWindowSize) {
            renderer->recreateSwapchain();
            updateWindowSize = false;
        }
        auto result = drawFrame(delta.count());
        if (result == 1) {
            renderer->recreateSwapchain();
        }
	}
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

std::vector<Path> App::drawNormals(std::shared_ptr<RenderObject> object)
{
    auto mesh = object->mesh;
    auto transform = object->transformMatrix.model;
    std::vector<Path> paths;
    for (const auto& vertex : mesh->_vertices) {
        auto start = transform * glm::vec4(vertex.pos, 1.0f);
        auto end = start + glm::vec4(vertex.normal, 1.0f) * 0.1f;
        paths.push_back(linePath(start, end, {0, 0, 1}));
    }
    return paths;
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
