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
#include <box2d/box2d.h>

export module DeluGame;
import xk.Math.Matrix;
import DeluEngine;
import SDL2pp;

class Paddle : public DeluEngine::GameObject, public DeluEngine::PulseCallback, public DeluEngine::RigidBody
{
private:
	//std::shared_ptr<DeluEngine::SpriteObject> m_sprite;
	float paddleDirection = 0;

public:
	Paddle(gsl::not_null<ECS::Scene*> scene, DeluEngine::SpriteObject::ConstructorParams spriteConstructor) :
		ECS::SceneAware{ scene },
		DeluEngine::GameObject{ scene },
		DeluEngine::PulseCallback{ "Game" },
		DeluEngine::RigidBody{ DeluEngine::BodyDef{ .type = b2BodyType::b2_kinematicBody, .allowSleep = false} }
		//m_sprite{ scene->NewObject<DeluEngine::SpriteObject>(spriteConstructor) }
	{
		//m_sprite->SetParent(this);
		SetLocalPosition({ 400, 200 });
		CreateAndRegisterFixture({.density = 1 }, DeluEngine::BoxShape{ { 100, 10 } });
		DeluEngine::Experimental::ControllerContext& context = GetEngine().controllerContext.FindContext("Game");
		DeluEngine::Experimental::ControllerAction& moveAction = context.FindAction("Player Move");
		moveAction.BindAxis2D([this](xk::Math::Aliases::Vector2 input) { paddleDirection = input.X(); });
	}

public:
	void Update(std::chrono::nanoseconds deltaTime) override
	{
		SetLocalPosition(GetLocalPosition() + xk::Math::Vector{ paddleDirection * std::chrono::duration<float>(deltaTime).count() * 50, 0.f });
	}

	void OnCollisionBegin(DeluEngine::Collision collision) override
	{
		if(collision.otherBody->GetParent())
			return;
		collision.otherBody->SetVelocity(xk::Math::Normalize(collision.otherBody->GetWorldPosition() - GetWorldPosition()) * 100);
	}
};

class Border : public DeluEngine::GameObject, public DeluEngine::RigidBody
{

public:
	Border(gsl::not_null<ECS::Scene*> scene) :
		ECS::SceneAware{ scene },
		DeluEngine::GameObject{ scene },
		DeluEngine::RigidBody{ DeluEngine::BodyDef{ .type = b2BodyType::b2_staticBody } }
	{
		DeluEngine::ChainShape shape;
		shape.loop = true;

		shape.vertices.push_back({ 0, 0 });
		shape.vertices.push_back({ 0, 600 });
		shape.vertices.push_back({ 1000, 600 });
		shape.vertices.push_back({ 1000, 0 });
		CreateAndRegisterFixture({}, shape);
	}
};

class Ball : public DeluEngine::GameObject, public DeluEngine::RigidBody
{

public:
	Ball(gsl::not_null<ECS::Scene*> scene) :
		ECS::SceneAware{ scene },
		DeluEngine::GameObject{ scene },
		DeluEngine::RigidBody{  BodyDef() }
	{
		CreateAndRegisterFixture({ .friction = 0, .restitution = 1, .density = 1}, DeluEngine::CircleShape{ .radius = 20 });
		//SetWorldPosition({200, 200});

		//SetVelocity({ 100, 100 });
	}

private:
	DeluEngine::BodyDef BodyDef()
	{
		DeluEngine::BodyDef def;
		def.bullet = true;
		def.linearDamping = 0;
		def.fixedRotation = true;
		def.gravityScale = 0;
		def.type = b2_dynamicBody;
		return def;
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
			std::shared_ptr<Paddle> paddle = scene.NewSceneOwnedObject<Paddle>(DeluEngine::SpriteObject::ConstructorParams{spriteData}).lock();
			scene.NewSceneOwnedObject<Border>();
			std::shared_ptr<Ball> ball = scene.NewSceneOwnedObject<Ball>().lock();
			ball->SetParent(paddle.get());
			ball->SetLocalPosition({ 50, 20 });

			DeluEngine::Experimental::ControllerContext& context = DeluEngine::GetEngine(scene).controllerContext.FindContext("Game");
			DeluEngine::Experimental::ControllerAction& moveAction = context.FindAction("Fire");
			moveAction.BindButton([weakBall = std::weak_ptr<Ball>(ball)](bool pressed)
				{ 
					auto ball = weakBall.lock();
					if(ball->GetParent())
					{
						ball->SetParent(nullptr);
						ball->SetVelocity({ 0, 100 });
					}
				});
			//scene.NewObject<ECS::GameObject>();
	};
}

export std::function<void(ECS::Scene&)> GameMain(DeluEngine::Engine& engine)
{
	std::srand(std::time(nullptr));

	{
		DeluEngine::Experimental::ControllerContext gameContext;

		{
			DeluEngine::Experimental::ControllerAction playerMove;
			playerMove.name = "Player Move";
			playerMove.invocationState = DeluEngine::KeyState::Held;
			DeluEngine::Experimental::Axis2D axis;
			axis.inputs.push_back({ DeluEngine::Key::Left, { -1, 0 } });
			axis.inputs.push_back({ DeluEngine::Key::Right, { 1, 0 } });
			playerMove.action = axis;
			gameContext.actions.push_back(playerMove);
		}

		{
			DeluEngine::Experimental::ControllerAction fireBall;
			fireBall.name = "Fire";
			fireBall.invocationState = DeluEngine::KeyState::Pressed;
			DeluEngine::Experimental::Button button;
			button.inputs.push_back({ DeluEngine::Key::Space });
			fireBall.action = button;
			gameContext.actions.push_back(fireBall);
		}

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
