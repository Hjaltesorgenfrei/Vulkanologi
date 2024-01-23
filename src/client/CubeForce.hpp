#ifndef GAME_H
#define GAME_H

#include "Physics.hpp"

struct Input {
	bool left;
	bool right;
	bool up;
	bool down;
	bool push;
	bool pull;

	Input() {
		left = false;
		right = false;
		up = false;
		down = false;
		push = false;
		pull = false;
	}

	bool operator==(const Input& other) {
		return left == other.left && right == other.right && up == other.up && down == other.down &&
			   push == other.push && pull == other.pull;
	}

	bool operator!=(const Input& other) { return !((*this) == other); }
};

extern void game_process_player_input(PhysicsWorld* world, const Input& input, float deltaTime, PhysicsBody body);

#endif  // #ifndef GAME_H