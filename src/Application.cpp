#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <iostream>
#include <functional>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>

#include "Renderer.hpp"
#include "Application.hpp"
#include "Cube.hpp"
#include "Sphere.hpp"
#include "Components.hpp"
#include "systems/CarSystem.hpp"
#include "systems/ControllerSystem.hpp"
#include "systems/TransformSystems.hpp"
#include "Colors.hpp"

void App::run() {
    instance = this;
    setupCallBacks(); // We create ImGui in the renderer, so callbacks have to happen before.
    device = std::make_unique<BehDevice>(window);
    AssetManager manager(device);
	renderer = std::make_unique<Renderer>(window, device, manager);
    physicsWorld = std::make_unique<PhysicsWorld>();
    setupWorld();
    mainLoop();
}

void App::setupCallBacks() {
    glfwSetWindowUserPointer(window->getGLFWwindow(), this);
    glfwSetFramebufferSizeCallback(window->getGLFWwindow(), framebufferResizeCallback);
    glfwSetCursorPosCallback(window->getGLFWwindow(), cursorPosCallback);
    glfwSetCursorEnterCallback(window->getGLFWwindow(), cursorEnterCallback);
    glfwSetMouseButtonCallback(window->getGLFWwindow(), mouseButtonCallback);
    glfwSetKeyCallback(window->getGLFWwindow(), keyCallback);
    glfwSetJoystickCallback(joystickCallback);
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
        for (auto entity : app->registry.view<SelectedTag>()) { // Deselect all before selecting new
            app->registry.remove<SelectedTag>(entity);
        }
        app->physicsWorld->closestRay(rayFromWorld, rayToWorld, [&](const btCollisionObject* body, const btVector3& point, const btVector3& normal) {
            if (body->getCollisionFlags() & btCollisionObject::CF_KINEMATIC_OBJECT) {
                return;
            }
            auto entity = static_cast<entt::entity>(body->getUserIndex());
            app->registry.emplace<SelectedTag>(entity);
        });

        glm::vec3 rayFromWorldGlm(rayFromWorld.x(), rayFromWorld.y(), rayFromWorld.z());
        glm::vec3 rayToWorldGlm(rayToWorld.x(), rayToWorld.y(), rayToWorld.z());
        rays.clear();
        rays.push_back(LinePath(rayFromWorldGlm, rayToWorldGlm, glm::vec3(1, 0, 0)));

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

    if(action == GLFW_PRESS) {
        for (auto [entity, input] : app->registry.view<KeyboardInput>().each()) {
            input.keys[key] = true;
        }
    }
    if(action == GLFW_RELEASE) {
        for (auto [entity, input] : app->registry.view<KeyboardInput>().each()) {
            input.keys[key] = false;
        }
    }
}

void App::joystickCallback(int joystickId, int event)
{
    auto * const app = App::instance;
    if (event == GLFW_CONNECTED && glfwJoystickIsGamepad(joystickId)) {
        app->addPlayer(GamepadInput { joystickId });
    }
    else if (event == GLFW_DISCONNECTED) {
        app->registry.view<GamepadInput>().each([&](auto entity, auto& input) {
            if (input.joystickId == joystickId) {
                app->registry.emplace<MarkForDeletionTag>(entity);
            }
        });
    }
}

template <typename T>
entt::entity App::addPlayer(T input)
{
    int playerId = 0;
    for (auto [entity, player] : registry.view<Player>().each()) {
        if (player.id >= playerId) {
            playerId = player.id + 1;
        }
    }
    std::vector<SpawnPoint> spawnPoints;
    for (auto [entity, spawnPoint] : registry.view<SpawnPoint>().each()) {
        spawnPoints.push_back(spawnPoint);
    }
    auto entity = registry.create();
    auto vehicle = physicsWorld->createVehicle();
    // Move vehicle to spawn point
    auto spawnPoint = spawnPoints[playerId % spawnPoints.size()];
    vehicle->getRigidBody()->getWorldTransform().setOrigin(btVector3(spawnPoint.position.x, spawnPoint.position.y, spawnPoint.position.z));
    auto rotation = glm::quatLookAt(spawnPoint.forward, glm::vec3(0, 1, 0));
    vehicle->getRigidBody()->getWorldTransform().setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));
    auto color = Color::playerColor(playerId);
    vehicle->getRigidBody()->setUserIndex((int)entity);
    registry.emplace<T>(entity, input);
    registry.emplace<Player>(entity, playerId, color);
    registry.emplace<Transform>(entity);
    registry.emplace<Car>(entity, vehicle);
    registry.emplace<CarStateLastUpdate>(entity);
    registry.emplace<CarControl>(entity);
    registry.emplace<std::shared_ptr<RenderObject>>(entity, std::make_shared<RenderObject>(carMesh, carMaterial));
    return entity;
}

void App::onRigidBodyDestroyed(entt::registry &registry, entt::entity entity)
{
    auto *rigidBody = registry.try_get<RigidBody>(entity);
    if (rigidBody == nullptr) {
        return;
    }
    physicsWorld->removeBody(rigidBody->body);
}

void App::onSensorDestroyed(entt::registry &registry, entt::entity entity)
{
    auto *sensor = registry.try_get<Sensor>(entity);
    if (sensor == nullptr) {
        return;
    }
    physicsWorld->removeGhost(sensor->ghost);
}

void App::onCarDestroyed(entt::registry &registry, entt::entity entity)
{
    auto *car = registry.try_get<Car>(entity);
    if (car == nullptr) {
        return;
    }
    physicsWorld->removeVehicle(car->vehicle);
}

float bytesToMegaBytes(uint64_t bytes) {
    return bytes / 1024.0f / 1024.0f;
}

// TODO: Move to circle path class
// Path circleAroundPoint(glm::vec3 center, float radius, int segments) {
//     Path path(true);
//     for (int i = 0; i < segments; i++) {
//         float angle = (float)i / (float)segments * 2.0f * 3.14159265359f;
//         glm::vec3 point = center + glm::vec3(cos(angle) * radius, sin(angle) * radius, 0);
//         path.addPoint({point, glm::vec3(1, 1, 1), glm::vec3(0, 1, 0)});
//     }
//     return path;
// }

enum Axis {
    X,
    Y,
    Z
};

Axis axis = X;

float splineValue(glm::vec3 pos, float minX, float maxX) {
    if (axis == Axis::X) {
        return (pos.x - minX) / (maxX - minX);
    }
    else if (axis == Axis::Y) {
        return (pos.y - minX) / (maxX - minX);
    }
    else {
        return (pos.z - minX) / (maxX - minX);
    }
}

float heightScale = 1.f;
float bumb = 1.f;

std::shared_ptr<Mesh> deformMesh(Bezier& bezier, std::shared_ptr<Mesh> baseMesh) {
    auto result = std::make_shared<Mesh>();
    result->_vertices = baseMesh->_vertices;
    result->_indices = baseMesh->_indices;
    result->_texturePaths = baseMesh->_texturePaths;
    result->_vertexBuffer = baseMesh->_vertexBuffer;
    result->_indexBuffer = baseMesh->_indexBuffer;

    auto& verts = result->_vertices;

    float min = FLT_MAX;
    float max = FLT_MIN;
    for (int i = 0; i < verts.size(); i++)
    {
        if (axis == Axis::X) {
            min = std::min(min, verts[i].pos.x);
            max = std::max(max, verts[i].pos.x);
        }
        else if (axis == Axis::Y) {
            min = std::min(min, verts[i].pos.y);
            max = std::max(max, verts[i].pos.y);
        }
        else {
            min = std::min(min, verts[i].pos.z);
            max = std::max(max, verts[i].pos.z);
        }
    }
    for (int i = 0; i < verts.size(); i++)
    {
        float sVal = splineValue(verts[i].pos, min, max);
        auto frameAt = bezier.frameAt(sVal);
        auto splinePosition = frameAt.o;
        auto splineNormal = frameAt.n;
        auto splineBinormal = frameAt.binormal();

        float offset = axis == Axis::X ? verts[i].pos.z : verts[i].pos.x;
        verts[i].pos = splineBinormal * offset * bumb + splinePosition + verts[i].pos.y * heightScale * splineNormal;
        verts[i].normal = frameAt.rotation() * glm::vec4(verts[i].normal, 1.0f);
    }
    return result;
}

int App::drawFrame(float delta) {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    FrameInfo frameInfo{};
    frameInfo.camera = camera;
    frameInfo.deltaTime = delta;

    for (auto entity : registry.view<std::shared_ptr<RenderObject>, Transform>()) {
        auto renderObject = registry.get<std::shared_ptr<RenderObject>>(entity);
        auto transform = registry.get<Transform>(entity);
        auto player = registry.try_get<Player>(entity);
        if (player != nullptr) {
            renderObject->transformMatrix.color = glm::vec4(player->color, 1.0f);
        }
        renderObject->transformMatrix.model = transform.modelMatrix;
        frameInfo.objects.push_back(renderObject);
    }

    // Make a imgui window to show all players
    std::vector<entt::entity> players;
    for (auto entity : registry.view<Player>()) {
        players.push_back(entity);
    }
    std::sort(players.begin(), players.end(), [&](entt::entity a, entt::entity b) {
        auto& playerA = registry.get<Player>(a);
        auto& playerB = registry.get<Player>(b);
        return playerA.id < playerB.id;
    });
    // Draw window without borders so it looks like a HUD
    ImGui::Begin("Players", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::SetWindowPos(ImVec2(0, 0));
    for (auto entity : players) {
        auto& player = registry.get<Player>(entity);
        ImVec4 color(player.color.x, player.color.y, player.color.z, 1.0f);
        ImGui::TextColored(color, "Player %d, lives: %d", player.id, player.lives);
        if (showDebugInfo) {
            ImGui::SameLine();
            ImGui::ColorEdit3("Color", &player.color[0]);
        }
    }
    ImGui::End();

    if(showDebugInfo) {
        drawFrameDebugInfo(delta, frameInfo);
    }
    
    auto result = renderer->drawFrame(frameInfo);
    ImGui::EndFrame();
    return result;
}

void App::drawFrameDebugInfo(float delta, FrameInfo& frameInfo)
{
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

    ImGui::Begin("Frame Info");
    ImGui::Text("Frame Time: %f", averageFrameTime);
    ImGui::Text("FPS: %f", 1000.0 / averageFrameTime);
    ImGui::Text("Memory Usage: %.1fmb", bytesToMegaBytes(memoryUsage));
    ImGui::End();

    registry.view<Bezier>().each([&](auto entity, Bezier& bezier) {
        bezier.recomputeIfDirty();
        frameInfo.paths.emplace_back(bezier);
        for (const auto point : bezier.getPoints()) {
            frameInfo.paths.emplace_back(LinePath(point.position, point.position + point.normal * 0.5f, {0, 0, 1}));
        }
        for (const auto frame : bezier.getFrenetFrames()) {
            auto start = frame.o;
            auto rightVector = glm::normalize(frame.r);
            auto end = frame.o + frame.t * 0.25f;
            auto right = frame.o + frame.t * 0.23f - rightVector * 0.01f;
            auto left = frame.o + frame.t * 0.23f + rightVector * 0.01f;
            frameInfo.paths.emplace_back(LinePath(start, end, {0, 1, 0}));
            // Make a arrow at the end
            frameInfo.paths.emplace_back(LinePath(end, right, {0, 1, 0}));
            frameInfo.paths.emplace_back(LinePath(end, left, {0, 1, 0}));
            frameInfo.paths.emplace_back(LinePath(right, left, {0, 1, 0}));
            frameInfo.paths.emplace_back(LinePath(start, start + frame.binormal() * 0.5f, {1, 0, 1}));
        }
    });

    for (auto entity : registry.view<SelectedTag>()) {
        drawDebugForSelectedEntity(entity, frameInfo);
    }
}

void App::drawRigidBodyDebugInfo(btRigidBody* body)
{
    auto transform = body->getWorldTransform();
    auto scale = body->getCollisionShape()->getLocalScaling();

    // Show with ImGui
    ImGui::Begin("Rigid Body Info");
    ImGui::Text("Position: %f, %f, %f", transform.getOrigin().x(), transform.getOrigin().y(), transform.getOrigin().z());
    ImGui::Text("Scale: %f, %f, %f", scale.x(), scale.y(), scale.z());
    ImGui::Text("Rotation: %f, %f, %f", transform.getRotation().x(), transform.getRotation().y(), transform.getRotation().z());
    ImGui::Text("Mass: %f", body->getInvMass());
    ImGui::Text("Friction: %f", body->getFriction());
    ImGui::Text("Restitution: %f", body->getRestitution());
    ImGui::Text("Linear Velocity: %f, %f, %f", body->getLinearVelocity().x(), body->getLinearVelocity().y(), body->getLinearVelocity().z());
    ImGui::Text("Angular Velocity: %f, %f, %f", body->getAngularVelocity().x(), body->getAngularVelocity().y(), body->getAngularVelocity().z());
    ImGui::Text("Linear Factor: %f, %f, %f", body->getLinearFactor().x(), body->getLinearFactor().y(), body->getLinearFactor().z());
    ImGui::Text("Angular Factor: %f, %f, %f", body->getAngularFactor().x(), body->getAngularFactor().y(), body->getAngularFactor().z());
    ImGui::Text("Gravity: %f, %f, %f", body->getGravity().x(), body->getGravity().y(), body->getGravity().z());
    ImGui::Text("Damping: %f, %f", body->getLinearDamping(), body->getAngularDamping());
    ImGui::Text("Sleeping: %s", body->isInWorld() ? "Yes" : "No");
    ImGui::Text("Kinematic: %s", body->isKinematicObject() ? "Yes" : "No");
    ImGui::Text("Static: %s", body->isStaticObject() ? "Yes" : "No");
    ImGui::Text("Active: %s", body->isActive() ? "Yes" : "No");
    ImGui::Text("Has Contact Response: %s", body->hasContactResponse() ? "Yes" : "No");

    if (ImGui::Button("Set Active")) {
        body->activate(true);
    }

    ImGui::End();
}

void App::drawDebugForSelectedEntity(entt::entity selectedEntity, FrameInfo& frameInfo)
{
    auto renderObject = registry.try_get<std::shared_ptr<RenderObject>>(selectedEntity);
    auto transform = registry.try_get<Transform>(selectedEntity);
    auto showNormals = registry.try_get<ShowNormalsTag>(selectedEntity);
    if (renderObject && transform && showNormals) {
        auto normals = drawNormals(*renderObject);
        frameInfo.paths.insert(frameInfo.paths.end(), normals.begin(), normals.end());
    }

    btRigidBody *rigidBody = nullptr;
    if (auto rigidBodyComponent = registry.try_get<RigidBody>(selectedEntity)) {
        rigidBody = rigidBodyComponent->body;
    }
    else if (auto car = registry.try_get<Car>(selectedEntity)) {
        rigidBody = car->vehicle->getRigidBody();
    }

    auto sensor = registry.try_get<Sensor>(selectedEntity);

    if (rigidBody) {
        drawRigidBodyDebugInfo(rigidBody);
    }       

    auto physicsPaths = physicsWorld->getDebugLines();
    frameInfo.paths.insert(frameInfo.paths.end(), physicsPaths.begin(), physicsPaths.end());
    frameInfo.paths.insert(frameInfo.paths.end(), rays.begin(), rays.end());

    if (rigidBody) {
        auto transform = rigidBody->getWorldTransform();
        auto scale = rigidBody->getCollisionShape()->getLocalScaling();
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
                rigidBody->getCollisionShape()->setLocalScaling(newScale);
            }
        }

        else if (drawImGuizmo(&modelMatrix)) {
            // convert back to bullet
            btTransform newTransform;
            newTransform.setFromOpenGLMatrix(glm::value_ptr(modelMatrix));
            rigidBody->setWorldTransform(newTransform);
        }

    }
    if (sensor != nullptr) {
        auto body = sensor->ghost;
        auto transform = body->getWorldTransform();
        auto scale = body->getCollisionShape()->getLocalScaling();
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
                body->getCollisionShape()->setLocalScaling(newScale);
            }
        }

        else if (drawImGuizmo(&modelMatrix)) {
            // convert back to bullet
            btTransform newTransform;
            newTransform.setFromOpenGLMatrix(glm::value_ptr(modelMatrix));
            body->setWorldTransform(newTransform);
        }
    }
}

void App::setupWorld() {
    std::vector<std::shared_ptr<RenderObject>> objects;
    // objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/lost_empire.obj"), Material{}));
    objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/road.obj")));
    objects.push_back(std::make_shared<RenderObject>(createCubeMesh()));
    objects.push_back(std::make_shared<RenderObject>(createCubeMesh()));
    objects.push_back(std::make_shared<RenderObject>(GenerateSphereSmooth(1, 10, 10)));
    objects.push_back(std::make_shared<RenderObject>(GenerateSphereSmooth(1, 10, 10)));
    objects.push_back(std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/na_bil.obj")));

    objects.back()->transformMatrix.model = glm::translate(glm::mat4(1), glm::vec3(5, 0, 0));

    registry.on_destroy<RigidBody>().connect<&App::onRigidBodyDestroyed>(this);
    registry.on_destroy<Sensor>().connect<&App::onSensorDestroyed>(this);
    registry.on_destroy<Car>().connect<&App::onCarDestroyed>(this);

    renderer->uploadMeshes(objects);
    carMesh = objects.back()->mesh;
    carMaterial = objects.back()->material;

    createSpawnPoints();

    for (int i = 0; i < 12; i++) {
        addPlayer(KeyboardInput {});
    }

    setupControllerPlayers();


    for (int i = 0; i < 0; i++) {
        auto entity = registry.create();
        entities.insert(entity);

        auto colShape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        auto startTransform = btTransform();
        startTransform.setIdentity();
        auto mass = 1.f;
        auto localInertia = btVector3(0, 0, 0);
        colShape->calculateLocalInertia(mass, localInertia);
        startTransform.setOrigin(btVector3(static_cast<btScalar>(5), 10, 2));
        auto myMotionState = new btDefaultMotionState(startTransform);
        auto rbInfo = btRigidBody::btRigidBodyConstructionInfo(mass, myMotionState, colShape, localInertia);
        auto body = new btRigidBody(rbInfo);
        body->setUserIndex((int)entity);
        physicsWorld->addBody(body);

        registry.emplace<Transform>(entity);
        registry.emplace<RigidBody>(entity, body);
        registry.emplace<std::shared_ptr<RenderObject>>(entity, objects[i]);
    }

    setupSystems();
    systemGraph.update(registry, 0.0f);
    physicsWorld->update(0.0f);

    auto beziers = registry.view<Bezier>();
    for (auto bezier : beziers) {
        auto road = std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/road.obj"));
        auto& bezierComponent = registry.get<Bezier>(bezier);
        // SplineMesh splineMesh = {Mesh::LoadFromObj("resources/road.obj")};
        // registry.emplace<SplineMesh>(bezier, splineMesh);
        bezierComponent.recomputeIfDirty();
        road->mesh = deformMesh(bezierComponent, road->mesh);
        registry.emplace<std::shared_ptr<RenderObject>>(bezier, road);
        registry.emplace<Transform>(bezier);
        // Make a bullet3 polygonal mesh
        std::vector<btVector3> vertices;
        for (auto index : road->mesh->_indices) {
            vertices.push_back(btVector3(road->mesh->_vertices[index].pos.x, road->mesh->_vertices[index].pos.y, road->mesh->_vertices[index].pos.z));
        }

        auto body = physicsWorld->createWorldGeometry(vertices);
        body->setUserIndex((int)bezier);
        registry.emplace<RigidBody>(bezier, body);
        renderer->uploadMeshes({road});
    }
    

    // Arena
    auto arena = std::make_shared<RenderObject>(Mesh::LoadFromObj("resources/arena.obj"));
    renderer->uploadMeshes({arena});
    auto entity = registry.create();
    entities.insert(entity);
    registry.emplace<Transform>(entity);
    registry.emplace<std::shared_ptr<RenderObject>>(entity, arena);
    std::vector<btVector3> vertices;
    for (auto index : arena->mesh->_indices) {
        vertices.push_back(btVector3(arena->mesh->_vertices[index].pos.x, arena->mesh->_vertices[index].pos.y, arena->mesh->_vertices[index].pos.z));
    }
    auto body = physicsWorld->createWorldGeometry(vertices);
    body->setUserIndex((int)entity);
    body->getCollisionShape()->setLocalScaling(btVector3(5, 5, 5));
    body->getWorldTransform().setOrigin(btVector3(0, -40, 60));
    registry.emplace<RigidBody>(entity, body);
}

void App::bezierTesting() {
    for (int i = 0; i < 2; i++){
        auto entity = registry.create();
        entities.insert(entity);
        // Create a ghost object using btGhostObject, same way i need to do control points
        btGhostObject* ghostObject = new btGhostObject();
        ghostObject->setWorldTransform(btTransform(btQuaternion(0, 0, 0, 1), btVector3(static_cast<btScalar>(-i * 10), static_cast<btScalar>(i * 4) - 1, static_cast<btScalar>(i * 2))));
        btConvexShape* sphere = new btSphereShape(0.1f);
        ghostObject->setCollisionShape(sphere);
        ghostObject->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
        ghostObject->setUserIndex((int)entity);
        physicsWorld->addGhost(ghostObject);
        registry.emplace<Transform>(entity);
        registry.emplace<Sensor>(entity, ghostObject);
        registry.emplace<ControlPointPtr>(entity);
    }

    std::vector<std::shared_ptr<ControlPoint>> controlPoints;

    registry.view<ControlPointPtr>().each([&](auto entity, auto& controlPoint) {
        controlPoints.push_back(controlPoint.controlPoint);
    });

    {
        auto entity = registry.create();
        entities.insert(entity);
        Bezier bezier(controlPoints, glm::vec3{1.f, 0.f, 0.f});
        registry.emplace<Bezier>(entity, bezier);
    }
}

void App::setupSystems()
{
    systemGraph.addSystem<CarKeyboardSystem>();
    systemGraph.addSystem<CarJoystickSystem>();
    systemGraph.addSystem<ControllerSystem>();
    systemGraph.addSystem<CarSystem>();
    systemGraph.addSystem<SensorTransformSystem>();
    systemGraph.addSystem<RigidBodySystem>();
    systemGraph.addSystem<CarTransformSystem>();
    systemGraph.addSystem<TransformControlPointsSystem>();
    
    systemGraph.init(registry);
    systemGraph.debugPrint();
}

void App::createSpawnPoints()
{
    // Read from text file SPAWNPOINTS.txt
    /* Format:
    3 // Number of spawn points
    0 0 0 // Spawn point 1 position
    1 0 0 // Spawn point 1 forward
    */
    std::ifstream file("resources/SPAWNPOINTS.txt");
    int numSpawnPoints;
    file >> numSpawnPoints;
    std::vector<SpawnPoint> spawnPoints;
    for (int i = 0; i < numSpawnPoints; i++) {
        float x, y, z;
        file >> x >> y >> z;
        glm::vec3 position = glm::vec3(x, y, z);
        file >> x >> y >> z;
        glm::vec3 forward = glm::vec3(x, y, z);
        spawnPoints.push_back(SpawnPoint{position, forward});
    }
    file.close();

    // Create spawn points
    for (int i = 0; i < numSpawnPoints; i++) {
        auto entity = registry.create();
        entities.insert(entity);
        registry.emplace<Transform>(entity);
        registry.emplace<SpawnPoint>(entity, spawnPoints[i]);
        btGhostObject* ghostObject = new btGhostObject();
        auto position = spawnPoints[i].position;
        auto forward = spawnPoints[i].forward;
        auto rotation = glm::quatLookAt(forward, glm::vec3(0, 1, 0));
        ghostObject->setWorldTransform(btTransform(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w), btVector3(static_cast<btScalar>(position.x), static_cast<btScalar>(position.y), static_cast<btScalar>(position.z))));
        btConvexShape* sphere = new btSphereShape(0.1f);
        ghostObject->setCollisionShape(sphere);
        ghostObject->setCollisionFlags(btCollisionObject::CF_NO_CONTACT_RESPONSE);
        ghostObject->setUserIndex((int)entity);
        physicsWorld->addGhost(ghostObject);
        registry.emplace<Sensor>(entity, ghostObject);
    }
}

void App::mainLoop() {
    auto timeStart = std::chrono::high_resolution_clock::now();

	while (!window->windowShouldClose()) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> delta = now - timeStart;
        timeStart = now;
		glfwPollEvents();
        processPressedKeys(delta.count());

        // TODO: Move this to a physics thread
        // TODO: Make this a fixed timestep
        
        physicsWorld->update(delta.count() / 1000.f);

        systemGraph.update(registry, delta.count());
        
        if (updateWindowSize) {
            renderer->recreateSwapchain();
            updateWindowSize = false;
        }
        auto result = drawFrame(delta.count());
        if (result == 1) {
            renderer->recreateSwapchain();
        }

        for (auto& entity : registry.view<MarkForDeletionTag>()) {
            registry.destroy(entity);
            entities.erase(entity);
        }
	}
}

void App::setupControllerPlayers()
{
    // Get all joysticks
    int joystickCount = 0;
    for (int joystickId = 0; joystickId < GLFW_JOYSTICK_LAST; joystickId++) {
        if (glfwJoystickPresent(joystickId)) {
            joystickCount++;
            const char* joystickName = glfwGetJoystickName(joystickId);
            std::cout << "Joystick " << joystickId << " is present: " << joystickName << std::endl;
            if (glfwJoystickIsGamepad(joystickId)) {
                std::cout << "Joystick " << joystickId << " is a gamepad" << std::endl;
                addPlayer(GamepadInput { joystickId });
            }
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

// TODO: Enable this by inspection with debug window.
std::vector<Path> App::drawNormals(std::shared_ptr<RenderObject> object)
{
    auto mesh = object->mesh;
    auto transform = object->transformMatrix.model;
    glm::mat4 view = glm::mat4(glm::mat3(transform));  
    std::vector<Path> paths;
    for (const auto& vertex : mesh->_vertices) {
        auto start = transform * glm::vec4(vertex.pos, 1.0f);
        auto end = start + (view * glm::vec4(vertex.normal, 1.0f)) * 0.1f;
        paths.push_back(LinePath(start, end, {0, 0, 1}));
    }
    return paths;
}

void App::processPressedKeys(float delta) {
    auto glfw_window = window->getGLFWwindow();
    float cameraSpeed = 0.005f * delta;
    if (shiftPressed) {
        cameraSpeed *= 4;
    }

    // TODO: Add camera as an entity which can take input :)
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
    catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        return EXIT_FAILURE;
    }

	return EXIT_SUCCESS;
}
