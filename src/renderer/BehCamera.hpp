#pragma once

#include <glm/glm.hpp>

class BehCamera {
public:
	BehCamera();

	glm::mat4 viewMatrix() const;

	void moveCameraForward(float speed);

	void moveCameraBackward(float speed);

	void moveCameraLeft(float speed);

	void moveCameraRight(float speed);

	void setCameraFront();

	void rotate(glm::vec2 rotation);

	glm::vec3 getCameraPosition() const;

	void setCameraPosition(const glm::vec3 &value, float delta);

	glm::vec3 getRayDirection(float xPos, float yPos, float width, float height) const;

	glm::mat4 getCameraProjection(float width, float height) const;

	void setTarget(glm::vec3 target, float targetSpeed, float delta);

	float speed = 0.006f;

private:
	glm::vec3 cameraPosition;
	glm::vec3 cameraFront;
	glm::vec3 cameraUp;
	glm::vec3 cameraTarget;
	float cameraYaw;
	float cameraPitch;
	bool hasTarget = false;

	float fovY = 45.0f;
};