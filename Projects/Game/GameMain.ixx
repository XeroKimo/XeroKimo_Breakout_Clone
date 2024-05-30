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

class Paddle : public DeluEngine::GameObject, public DeluEngine::PulseCallback
{
private:
	std::shared_ptr<DeluEngine::SpriteObject> m_sprite;
	float paddleDirection = 0;

public:
	Paddle(gsl::not_null<ECS::Scene*> scene, DeluEngine::SpriteObject::ConstructorParams spriteConstructor) :
		ECS::SceneAware{ scene },
		DeluEngine::GameObject{ scene },
		PulseCallback{ "Game" },
		m_sprite{ scene->NewObject<DeluEngine::SpriteObject>(spriteConstructor) }
	{
		m_sprite->SetParent(this);
		SetLocalPosition({ 20, 256 });

		DeluEngine::Experimental::ControllerContext& context = GetEngine().controllerContext.FindContext("Game");
		DeluEngine::Experimental::ControllerAction& moveAction = context.FindAction("Player Move");
		moveAction.BindAxis2D([this](xk::Math::Aliases::Vector2 input) { paddleDirection = input.X(); });
	}

public:
	void Update(std::chrono::nanoseconds deltaTime) override
	{
		SetLocalPosition(GetLocalPosition() + xk::Math::Vector{ paddleDirection * std::chrono::duration<float>(deltaTime).count() * 50, 0.f });
	}


};

auto TestScene()
{
	return [](ECS::Scene& scene)
	{
			DeluEngine::Engine& engine = DeluEngine::GetEngine(scene);
			engine.controllerContext.ClearContextStack();
			engine.controllerContext.PushContext("Game");

			SDL2pp::view_ptr<SDL2pp::Renderer> renderer = DeluEngine::GetEngine(scene).renderer.backend.get();
			SDL2pp::unique_ptr<SDL2pp::Surface> surface{ IMG_Load("BlankCard.png") };
			std::shared_ptr<DeluEngine::SpriteData> spriteData = std::make_shared<DeluEngine::SpriteData>();
			spriteData->texture = renderer->CreateTexture(surface.get());
			spriteData->drawRect.x = 0;
			spriteData->drawRect.y = 0;
			spriteData->drawRect.w = surface.get()->w;
			spriteData->drawRect.h = surface.get()->h;
			scene.NewSceneOwnedObject<Paddle>(DeluEngine::SpriteObject::ConstructorParams{spriteData});
			//scene.NewObject<ECS::GameObject>();
	};
}

export std::function<void(ECS::Scene&)> GameMain(DeluEngine::Engine& engine)
{
	std::srand(std::time(nullptr));

	{
		DeluEngine::Experimental::ControllerContext gameContext;
		DeluEngine::Experimental::ControllerAction playerMove;
		playerMove.name = "Player Move";
		playerMove.invocationState = DeluEngine::KeyState::Held;
		DeluEngine::Experimental::Axis2D axis;
		axis.inputs.push_back({ DeluEngine::Key::Left, { -1, 0 } });
		axis.inputs.push_back({ DeluEngine::Key::Right, { 1, 0 } });
		playerMove.action = axis;
		gameContext.actions.push_back(playerMove);

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
