#include <box2d/box2d.h>
#include <memory>

module DeluEngine:Physics;
import DeluEngine;

namespace DeluEngine
{
	//template<>
	//b2World& PhysicsContainer<PhysicsGameObject>::GetPhysicsWorld()
	//{
	//	return Self().GetEngine().physicsWorld;
	//}


	std::weak_ptr<Fixture> RigidBody::CreateAndRegisterFixture(const FixtureParams& params, const CircleShape& shape)
	{
		m_fixtures.push_back(GetScene().NewObject<CircleFixture>(this, params, shape));
		return m_fixtures.back();
	}

	std::weak_ptr<Fixture> RigidBody::CreateAndRegisterFixture(const FixtureParams& params, const BoxShape& shape)
	{
		m_fixtures.push_back(GetScene().NewObject<BoxFixture>(this, params, shape));
		return m_fixtures.back();
	}

	std::weak_ptr<Fixture> RigidBody::CreateAndRegisterFixture(const FixtureParams& params, const ChainShape& shape)
	{
		m_fixtures.push_back(GetScene().NewObject<ChainFixture>(this, params, shape));
		return m_fixtures.back();
	}

	b2World& RigidBody::GetPhysicsWorld()
	{
		return GetEngine(GetScene()).physicsWorld;
	}
}