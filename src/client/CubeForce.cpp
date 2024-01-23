#include "CubeForce.hpp"

static inline glm::vec3 transform(const glm::quat& quat, const glm::vec3& input) {
	glm::quat quat_inv = conjugate(quat);
	glm::quat a(input.x, input.y, input.z, 0);
	glm::quat r = (quat * a) * quat_inv;
	return glm::vec3(r.x, r.y, r.z);
}

static glm::quat axis_rotation(float angle_radians, glm::vec3 axis) {
	const float a = angle_radians * 0.5f;
	const float s = (float)sin(a);
	const float c = (float)cos(a);
	return glm::quat(axis.x * s, axis.y * s, axis.z * s, c);
}

void game_process_player_input(PhysicsWorld* world, const Input& input, float t, PhysicsBody body) {
	const float strafe_force = 12000.0f;

	float force_x = 0.0f;
	float force_z = 0.0f;

	force_x += strafe_force * (int)input.left;
	force_x -= strafe_force * (int)input.right;
	force_z += strafe_force * (int)input.up;
	force_z -= strafe_force * (int)input.down;

	glm::vec3 force(force_x, 0.0f, force_z);

	if (!input.push) {
		if (glm::dot(body.velocity, force) < 100.0f) force *= 2.0f;
	}

	world->addForce(body.bodyID, force);

	if (input.push) {
		// bobbing force on player cube
		{
			const double wobble_x = sin(t * 0.1 + 1) + sin(t * 0.05f + 3) + sin(t + 10);
			const double wobble_y = sin(t * 0.1 + 2) + sin(t * 0.05f + 4) + sin(t + 11);
			const double wobble_z = sin(t * 0.1 + 3) + sin(t * 0.05f + 5) + sin(t + 12);
			glm::vec3 force = glm::vec3(wobble_x, wobble_y, wobble_z) * 2.0f;
			world->addForce(body.bodyID, force);
		}

		// bobbing torque on player cube
		{
			const double wobble_x = sin(t * 0.05 + 10) + sin(t * 0.05f + 22) + sin(t * 1.1f);
			const double wobble_y = sin(t * 0.09 + 5) + sin(t * 0.045f + 16) + sin(t * 1.11f);
			const double wobble_z = sin(t * 0.05 + 4) + sin(t * 0.055f + 9) + sin(t * 1.12f);
			glm::vec3 torque = glm::vec3(wobble_x, wobble_y, wobble_z) * 1.5f;
			world->addTorque(body.bodyID, torque);
		}

		// calculate velocity tilt for player cube
		glm::vec3 target_up(0, 0, 1);
		{
			glm::vec3 strafe(force.x, 0, force.z);
			if (glm::length(strafe) * glm::length(strafe)) {
				glm::vec3 axis = normalize(cross(-force, glm::vec3(0, 1, 0)));
				float tilt = 0.2 + 0.1f * glm::length(body.velocity);
				if (tilt > 0.4f) tilt = 0.4f;
				target_up = transform(axis_rotation(tilt, axis), target_up);
			}
		}

		// stay upright torque on player cube
		{
			glm::vec3 current_up = transform(body.rotation, glm::vec3(0, 1, 0));
			glm::vec3 axis = cross(target_up, current_up);
			float angle = acos(dot(target_up, current_up));
			if (angle > 0.5f) angle = 0.5f;
			glm::vec3 torque = axis * angle * -100.f;
			world->addTorque(body.bodyID, torque);
		}
	}
}