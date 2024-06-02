module;

#include <box2d/box2d.h>
#include <cassert>
#include <memory>
#include <vector>

export module DeluEngine:Physics;
import :ECS;
import :ForwardDeclares;
import xk.Math.Matrix;
import xk.Math.Angles;

namespace DeluEngine
{
	export struct BodyDef
	{
		b2BodyType type = b2_staticBody;
		b2Vec2 linearVelocity = { 0, 0 };
		float angularVelocity = 0;
		float linearDamping = 0;
		float angularDamping = 0;
		bool allowSleep = true;
		bool awake = true;
		bool fixedRotation = false;
		bool bullet = false;
		bool enabled = true;
		float gravityScale = 1;

		operator b2BodyDef() const 
		{
			b2BodyDef def;
			def.type = type;
			def.linearVelocity = linearVelocity;
			def.angularVelocity = angularVelocity;
			def.linearDamping = linearDamping;
			def.angularDamping = angularDamping;
			def.allowSleep = allowSleep;
			def.awake = awake;
			def.fixedRotation = fixedRotation;
			def.bullet = bullet;
			def.enabled = enabled;
			def.gravityScale = gravityScale;

			return def;
		}
	};

	export struct FixtureParams
	{
		float friction = 0.2f;
		float restitution = 0;
		float restitutionThreshold = 1.0f * b2_lengthUnitsPerMeter;
		float density = 0.f;
		bool isSolid = true;
		b2Filter filter;

		operator b2FixtureDef() const noexcept
		{
			b2FixtureDef def;
			def.friction = friction;
			def.restitution = restitution;
			def.restitutionThreshold = restitutionThreshold;
			def.density = density;
			def.isSensor = !isSolid;
			def.filter = filter;
			return def;
		}
	};

	export struct CircleShape
	{
		float radius;

		operator b2CircleShape() const noexcept
		{
			b2CircleShape shape{};
			shape.m_radius = radius;
			return shape;
		}
	};

	export struct BoxShape
	{
		xk::Math::Aliases::Vector2 size;

		operator b2PolygonShape() const noexcept
		{
			b2PolygonShape shape{};
			shape.SetAsBox(size.X(), size.Y());
			return shape;
		}
	};

	export class RigidBody;


	//TODO: Instead of offering an inheritence tree for fixtures, maybe instead look to just having a base fixture
	//and offering views which indicate the kind of fixture it is. This would simplify the behaviour of the class a lot
	//TODO: Prevent fixtures from being created from a direct call to NewObject. We could use a struct with private constructor +
	//friend class of RigidBody to force all construction and deconstruction to go through the RigidBody class
	export class Fixture : 
		public ECS::Object,
		public virtual ECS::TransformNode
	{
	private:
		gsl::not_null<RigidBody*> m_owner;
		b2Fixture* m_fixture;

	protected: 
		Fixture(gsl::not_null<ECS::Scene*> scene, gsl::not_null<RigidBody*> owner, const FixtureParams& params, const CircleShape& shape);
		Fixture(gsl::not_null<ECS::Scene*> scene, gsl::not_null<RigidBody*> owner, const FixtureParams& params, const BoxShape& shape);


		~Fixture() override;
	};

	export class BoxFixture;
	export class CircleFixture;

	export class RigidBody : 
		public virtual ECS::SceneAware, 
		public virtual ECS::TransformNode
	{
		friend class Fixture;

	private:
		b2Body* m_body = nullptr;
		std::vector<std::shared_ptr<Fixture>> m_fixtures;

	public:
		RigidBody(const BodyDef& definition) :
			ECS::SceneAware(ECS::VirtualInheritencePassthrough{})
		{
			CreateBody(definition);
		}

	protected:
		~RigidBody()
		{
			m_fixtures.clear();
			GetPhysicsWorld().DestroyBody(m_body);
		}

	public:
		std::weak_ptr<Fixture> CreateAndRegisterFixture(const FixtureParams& params, const CircleShape& shape);

		std::weak_ptr<Fixture> CreateAndRegisterFixture(const FixtureParams& params, const BoxShape& shape);

		void RemoveFixture(std::weak_ptr<Fixture> fixture)
		{
			std::erase(m_fixtures, fixture.lock());
		}

		void SyncTransformRigidBodyToInternalBody()
		{
			m_body->SetTransform({ GetWorldPosition().X(), GetWorldPosition().Y() }, GetWorldRotation()._value);
		}

		void SyncTransformInternalBodyToRigidBody()
		{
			b2Transform t = m_body->GetTransform();
			SetWorldPosition(xk::Math::Vector<float, 2>{ t.p.x, t.p.y });
			SetWorldRotation(xk::Math::Degree<float>{ t.q.GetAngle() });
		}

	private:
		void CreateBody(const BodyDef& definition)
		{
			b2BodyDef def = definition;
			b2BodyUserData data;

			def.position = { GetWorldPosition().X(), GetWorldPosition().Y() };
			def.angle = GetWorldRotation()._value;
			data.pointer = reinterpret_cast<uintptr_t>(this);
			def.userData = data;
			m_body = GetPhysicsWorld().CreateBody(&def);
		}

		b2World& GetPhysicsWorld();
	};

	export class BoxFixture : public Fixture
	{
	public:
		BoxFixture(gsl::not_null<ECS::Scene*> scene, gsl::not_null<RigidBody*> owner, const FixtureParams& params, const BoxShape& shape) :
			ECS::SceneAware{ scene },
			ECS::TransformNode{ {}, owner.get(), ECS::ReparentLogic::KeepLocalTransform },
			Fixture{ scene, owner, params, shape }
		{

		}
	};

	export class CircleFixture : public Fixture
	{
	public:
		CircleFixture(gsl::not_null<ECS::Scene*> scene, gsl::not_null<RigidBody*> owner, const FixtureParams& params, const CircleShape& shape) :
			ECS::SceneAware{ scene },
			ECS::TransformNode{ {}, owner.get(), ECS::ReparentLogic::KeepLocalTransform },
			Fixture{ scene, owner, params, shape }
		{

		}
	};

	Fixture::Fixture(gsl::not_null<ECS::Scene*> scene, gsl::not_null<RigidBody*> owner, const FixtureParams& params, const CircleShape& shape) :
		ECS::Object{ scene },
		ECS::SceneAware{ ECS::VirtualInheritencePassthrough{} },
		m_owner{ owner }
	{
		b2FixtureDef def = params;
		b2CircleShape circle = shape;
		circle.m_p = { GetLocalPosition().X(), GetLocalPosition().Y() };
		b2FixtureUserData data;
		data.pointer = reinterpret_cast<std::uintptr_t>(this);
		def.shape = &circle;
		def.userData = data;
		m_fixture = m_owner->m_body->CreateFixture(&def);
	}

	Fixture::Fixture(gsl::not_null<ECS::Scene*> scene, gsl::not_null<RigidBody*> owner, const FixtureParams& params, const BoxShape& shape) :
		ECS::Object{ scene },
		ECS::SceneAware{ ECS::VirtualInheritencePassthrough{} },
		m_owner{ owner }
	{
		b2FixtureDef def = params;
		b2PolygonShape box;
		box.SetAsBox(shape.size.X(), shape.size.Y(), { GetLocalPosition().X(), GetLocalPosition().Y() }, GetLocalRotation()._value);
		b2FixtureUserData data;
		data.pointer = reinterpret_cast<std::uintptr_t>(this);
		def.shape = &box;
		def.userData = data;
		m_fixture = m_owner->m_body->CreateFixture(&def);
	}

	Fixture::~Fixture()
	{
		m_owner->m_body->DestroyFixture(m_fixture);
	}
}
//
//namespace DeluEngine
//{
//	export struct BodyDef
//	{
//		b2BodyType type = b2_staticBody;
//		b2Vec2 linearVelocity = { 0, 0 };
//		float angularVelocity = 0;
//		float linearDamping = 0;
//		float angularDamping = 0;
//		bool allowSleep = true;
//		bool awake = true;
//		bool fixedRotation = false;
//		bool bullet = false;
//		bool enabled = true;
//		float gravityScale = 1;
//
//		operator b2BodyDef() const 
//		{
//			b2BodyDef def;
//			def.type = type;
//			def.linearVelocity = linearVelocity;
//			def.angularVelocity = angularVelocity;
//			def.linearDamping = linearDamping;
//			def.angularDamping = angularDamping;
//			def.allowSleep = allowSleep;
//			def.awake = awake;
//			def.fixedRotation = fixedRotation;
//			def.bullet = bullet;
//			def.enabled = enabled;
//			def.gravityScale = gravityScale;
//
//			return def;
//		}
//	};
//
//	export struct Engine;
//	//Assumes to be derived from DeluEngine::GameObject
//	export template<class DerivedSelf>
//	class PhysicsContainer
//	{
//	private:
//		b2Body* m_body = nullptr;
//
//	public:
//		PhysicsContainer() = default;
//		PhysicsContainer(BodyDef def)
//		{
//			b2BodyDef internalDef = def;
//			internalDef.position = { Self().Transform().Position().GetValue().X(), Self().Transform().Position().GetValue().Y() };
//			internalDef.angle = Self().Transform().Rotation().GetValue()._value;
//			b2BodyUserData data;
//			data.pointer = reinterpret_cast<uintptr_t>(static_cast<GameObject*>(&Self()));
//			internalDef.userData = data;
//
//			m_body = GetPhysicsWorld().CreateBody(&internalDef);
//		}
//
//		~PhysicsContainer()
//		{
//			if (m_body)
//			{
//				GetPhysicsWorld().DestroyBody(m_body);
//			}
//		}
//
//	public:
//		b2Fixture* AddFixture(const b2Shape& shape, float density = 0)
//		{
//			b2FixtureDef def; 
//			def.shape = &shape;
//			def.density = density;
//			return AddFixture(def);
//		}
//
//		b2Fixture* AddFixture(const b2FixtureDef& fixture)
//		{
//			CreateDefaultBodyIfNull();
//			return m_body->CreateFixture(&fixture);
//		}
//
//		void RemoveFixture(const b2FixtureDef* fixture)
//		{
//			if (!m_body)
//				return;
//			return m_body->DestroyFixture(fixture);
//		}
//
//		void SyncTransformGameObjectToBody()
//		{
//			if (!m_body)
//				return;
//			m_body->SetTransform({ Self().Transform().Position().GetValue().X(), Self().Transform().Position().GetValue().Y() }, Self().Transform().Rotation().GetValue()._value);
//		}
//
//		void SyncTransformBodyToGameObject()
//		{
//			if (!m_body)
//				return;
//			b2Transform t = m_body->GetTransform();
//			Self().Transform().Position() = xk::Math::Vector<float, 2>{ t.p.x, t.p.y };
//			Self().Transform().Rotation() = xk::Math::Degree<float>{ t.q.GetAngle() };
//		}
//
//	private:
//		void CreateDefaultBodyIfNull()
//		{
//			if (!m_body)
//			{
//				b2BodyDef def;
//				b2BodyUserData data;
//
//				def.position = { Self().Transform().Position().GetValue().X(), Self().Transform().Position().GetValue().Y() };
//				def.angle = Self().Transform().Rotation().GetValue()._value;
//				data.pointer = reinterpret_cast<uintptr_t>(static_cast<GameObject*>(&Self()));
//				def.userData = data;
//				m_body = GetPhysicsWorld().CreateBody(&def);
//			}
//		}
//		DerivedSelf& Self() { return static_cast<DerivedSelf&>(*this); }
//		const DerivedSelf& Self() const { return static_cast<DerivedSelf&>(*this); }
//		b2World& GetPhysicsWorld();
//	};
//
//	template<class DerivedSelf>
//	b2World& PhysicsContainer<DerivedSelf>::GetPhysicsWorld() { return Self().GetEngine().physicsWorld; }
//
//	export class PhysicsGameObject : public GameObject, public PhysicsContainer<PhysicsGameObject>
//	{
//	private:
//		bool m_userTransformChanged = true;
//
//	public:
//		using ObjectBaseClasses = ECS::ObjectBaseClassesHelper<GameObject>;
//
//	public:
//		PhysicsGameObject(const ECS::ObjectInitializer& objInitializer, const ECS::UserGameObjectInitializer& initializer, BodyDef bodyDef = {}) :
//			GameObject(objInitializer, initializer),
//			PhysicsContainer<PhysicsGameObject>(bodyDef)
//		{
//
//		}
//
//		void Update(float deltaTime) override
//		{
//			if (m_userTransformChanged)
//			{
//				SyncTransformGameObjectToBody();
//				m_userTransformChanged = false;
//			}
//			else
//			{
//				SyncTransformBodyToGameObject();
//				m_userTransformChanged = false;
//			}
//		}
//
//		ECS::LocalTransformProxy LocalTransform() override
//		{
//			m_userTransformChanged = true;
//			return ECS::GameObject::LocalTransform();
//		}
//
//		ECS::WorldTransformProxy Transform() override
//		{
//			m_userTransformChanged = true;
//			return ECS::GameObject::Transform();
//		}
//
//		void SetParent(ECS::GameObject* parent) override
//		{
//			m_userTransformChanged = true;
//			ECS::GameObject::SetParent(parent);
//		}
//	};
//
//	template<>
//	b2World& PhysicsContainer<PhysicsGameObject>::GetPhysicsWorld();
//};