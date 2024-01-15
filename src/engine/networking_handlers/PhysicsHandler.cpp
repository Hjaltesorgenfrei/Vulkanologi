#include "PhysicsHandler.hpp"

#include <glm/glm.hpp>

const void PhysicsHandler::internalHandle(PhysicsState* state) {
	printf("tick: %d, entities: %d\n", state->tick, state->entities);
	const int blockSize = state->GetBlockSize();
	const uint8_t* blockData = state->GetBlockData();
	for (uint32_t i = 0; i < state->entities; i++) {
		glm::vec3 position;
		memcpy(&position, blockData, sizeof(glm::vec3));
		blockData += sizeof(glm::vec3);
		printf("entity %d: position: (%f, %f, %f)\n", i, position.x, position.y, position.z);
	}
}
