#pragma once
#include "Renderer.h"
#include "Model.h"

class App {
public:
	void run();

private:
	std::shared_ptr<WindowWrapper> window;
	std::shared_ptr<Model> model;
	std::unique_ptr<Renderer> renderer;

	void mainLoop();

};
