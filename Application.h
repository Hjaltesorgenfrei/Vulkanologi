#pragma once
#include "Renderer.h"

class App {
public:
	void run();

private:
	std::shared_ptr<Window> window;
	std::unique_ptr<Renderer> renderer;

	void mainLoop();

};
