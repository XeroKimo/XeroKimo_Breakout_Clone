module;

#include <memory>
#include <chrono>

export module DeluEngine:SpriteComponent;
import :ForwardDeclares;
import :ECS;
//export import :ECS;
import :Heart;
import :Renderer;

namespace DeluEngine
{
	export class SpriteObject : public GameObject, public PulseCallback
	{
	public:
		struct ConstructorParams
		{
			std::shared_ptr<SpriteData> sprite = nullptr;
		};

	private:
		SpriteHandle m_spriteHandle;

	//public:
	//	using ObjectBaseClasses = ECS::ObjectBaseClassesHelper<DeluEngine::GameObject>;

	public:
		//SpriteObject(const ECS::ObjectInitializer& initializer, const ECS::UserGameObjectInitializer& goInitializer, ConstructorParams params = {});
		SpriteObject(gsl::not_null<ECS::Scene*> scene, ConstructorParams params = {});

	public:
		void Update(std::chrono::nanoseconds deltaTime) override
		{
			//m_spriteHandle->position = Transform().Position();
			//m_spriteHandle->angle = static_cast<xk::Math::Degree<float>>(Transform().Rotation());
			m_spriteHandle->position = GetWorldPosition();
			m_spriteHandle->angle = GetWorldRotation();
		}
	};
}