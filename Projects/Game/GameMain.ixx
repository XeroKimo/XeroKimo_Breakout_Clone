module;

#include <functional>
#include <vector>
#include <memory>
#include <chrono>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <array>
#include <span>
#include <cstdlib>
#include <ctime>
#include <format>
#include <SDL2/SDL_mixer.h>

export module DeluGame;
import xk.Math.Matrix;
import DeluEngine;
import SDL2pp;

auto TestScene()
{
	return [](ECS::Scene& scene)
	{
	};
}

export std::function<void(ECS::Scene&)> GameMain(DeluEngine::Engine& engine)
{
	std::srand(std::time(nullptr));

	{
		DeluEngine::Experimental::ControllerContext gameContext;
		DeluEngine::Experimental::ControllerAction pause;
		pause.name = "Pause";
		pause.invocationState = DeluEngine::KeyState::Pressed;
		pause.action = DeluEngine::Experimental::Button{ .inputs{{ DeluEngine::Key::Escape }} };
		gameContext.actions.push_back(pause);

		engine.controllerContext.RegisterContext("Game", gameContext);
	}
	{
		DeluEngine::Experimental::ControllerContext pauseContext;
		DeluEngine::Experimental::ControllerAction resume;
		resume.name = "Resume";
		resume.invocationState = DeluEngine::KeyState::Pressed;
		resume.action = DeluEngine::Experimental::Button{ .inputs{ {DeluEngine::Key::Escape }} };
		pauseContext.actions.push_back(resume);

		engine.controllerContext.RegisterContext("Pause", pauseContext);
	}

	return TestScene();
}
