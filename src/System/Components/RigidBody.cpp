#include "precompile.h"
#include "RigidBody.h"

using namespace physx;
void RigidBody::Construct()
{
	status.status_bit.on(CompStat::STATUS::SINGLE);
	SetPriority(0);
}
int RigidBody::Init()
{

	auto p_scene = owner->GetScene()->GetPhysicsScene();
	body
		= PhysicsManager::GetPhysicsInstance()->createRigidDynamic(physx::PxTransform(physx::PxIdentity));
	body->userData = new SafeWeakPtr<Object>(owner->shared_from_this());

	p_scene->addActor(*body);
	if (is_kinematic)
		return 0;
	if (auto rig_body = body->is<PxRigidDynamic>()) {
		rig_body->setLinearVelocity(velocity);
		rig_body->setAngularVelocity(velocity);
	}

	return 0;
}

void RigidBody::PrePhysics()
{
	auto& owner_trns = owner->transform;
	PxTransform trns(owner_trns->position + owner_trns->rotation.rotate(pos), owner_trns->rotation * rot);
	body->setGlobalPose(trns);
	if (body->is<PxRigidDynamic>()) {
		auto rig_body = static_cast<PxRigidDynamic*>(body);
		if (!is_kinematic) {
			rig_body->setLinearVelocity(velocity);
			rig_body->setAngularVelocity(angular_velocity);
		}
		rig_body->setMass(mass);
		rig_body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_X, freeze_position.x);
		rig_body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Y, freeze_position.y);
		rig_body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, freeze_position.z);
		rig_body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, freeze_rotation.x);
		rig_body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, freeze_rotation.y);
		rig_body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Z, freeze_rotation.z);
		rig_body->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !use_gravity);
		rig_body->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, is_kinematic);

	}
}

void RigidBody::PostPhysics()
{
	auto& owner_trns = owner->transform;
	auto trns = body->getGlobalPose();
	owner->transform->SetPosition(trns.p - trns.q.rotate(Vector3(pos)));
	owner->transform->SetRotation(trns.q);
	if (is_kinematic)
		return;
	if (auto rig_body = body->is<PxRigidDynamic>()) {
		velocity = rig_body->getLinearVelocity();
		angular_velocity = rig_body->getAngularVelocity();
	}

}

void RigidBody::Update()
{
}

void RigidBody::DebugDraw()
{
	DrawSphere3D(cast(body->getGlobalPose().p), 0.1f, 8, Color::YELLOW, Color::YELLOW, true);
}

void RigidBody::Exit()
{

	owner->GetScene()->DeleteActor(body);
	body = nullptr;
}

void RigidBody::AddForce(Vector3 force, ForceMode force_mode)
{
	if (!body->is<PxRigidDynamic>())
		return;
	bool in_simulation = SceneManager::GetCurrentScene()->IsInSimulation();
	if (!in_simulation && !is_kinematic) {
		static_cast<PxRigidDynamic*>(body)->addForce(force, static_cast<PxForceMode::Enum>(force_mode));
		return;
	}
	auto lambda = [wp = SafeWeakPtr<RigidBody>(std::static_pointer_cast<RigidBody>(shared_from_this())), force, force_mode]() {
		if (!wp)
			return;
		if (wp->is_kinematic)
			return;
		if (static_cast<SafeWeakPtr<Object>*>(wp->body->userData))
			static_cast<PxRigidDynamic*>(wp->body)->addForce(force, static_cast<PxForceMode::Enum>(force_mode));
		};
	SceneManager::GetCurrentScene()->AddFunctionAfterSimulation(lambda);

}

void RigidBody::AddTorque(Vector3 torque, ForceMode force_mode)
{
	if (!body->is<PxRigidDynamic>())
		return;
	bool in_simulation = SceneManager::GetCurrentScene()->IsInSimulation();
	if (!in_simulation && !is_kinematic) {
		static_cast<PxRigidDynamic*>(body)->addTorque(torque, static_cast<PxForceMode::Enum>(force_mode));
		return;
	}
	auto lambda = [wp = SafeWeakPtr<RigidBody>(std::static_pointer_cast<RigidBody>(shared_from_this())), torque, force_mode]() {
		if (!wp)
			return;
		if (wp->is_kinematic)
			return;
		if (static_cast<SafeWeakPtr<Object>*>(wp->body->userData))
			static_cast<PxRigidDynamic*>(wp->body)->addTorque(torque, static_cast<PxForceMode::Enum>(force_mode));
		};
	SceneManager::GetCurrentScene()->AddFunctionAfterSimulation(lambda);
}

void RigidBody::AddForceAtPosition(Vector3 force, Vector3 world_position, ForceMode force_mode)
{
	if (!body->is<PxRigidDynamic>())
		return;
	bool in_simulation = SceneManager::GetCurrentScene()->IsInSimulation();
	if (!in_simulation && !is_kinematic) {
		static_cast<PxRigidDynamic*>(body)->addForce(force, static_cast<PxForceMode::Enum>(force_mode));
		auto cross = (world_position - body->getGlobalPose().p).cross(force);
		static_cast<PxRigidDynamic*>(body)->addTorque(cross, static_cast<PxForceMode::Enum>(force_mode));
		return;
	}
	auto lambda = [wp = SafeWeakPtr<RigidBody>(std::static_pointer_cast<RigidBody>(shared_from_this())), force, world_position, force_mode]() {
		if (!wp)
			return;
		if (wp->is_kinematic)
			return;
		if (static_cast<SafeWeakPtr<Object>*>(wp->body->userData)) {
			static_cast<PxRigidDynamic*>(wp->body)->addForce(force, static_cast<PxForceMode::Enum>(force_mode));
			static_cast<PxRigidDynamic*>(wp->body)->addTorque((world_position - wp->body->getGlobalPose().p).cross(force), static_cast<PxForceMode::Enum>(force_mode));
		}
		};
	SceneManager::GetCurrentScene()->AddFunctionAfterSimulation(lambda);
}

void RigidBody::SetMassCenter(Vector3 center)
{
	if (body->is<PxRigidDynamic>())
		static_cast<PxRigidDynamic*>(body)->setCMassLocalPose(PxTransform(center));
}

void RigidBody::SetVelocity(Vector3 velocity_)
{
	velocity = velocity_;
	bool in_simulation = SceneManager::GetCurrentScene()->IsInSimulation();
	if (!in_simulation && !is_kinematic) {
		if (body->is<PxRigidDynamic>())
			static_cast<PxRigidDynamic*>(body)->setLinearVelocity(velocity_);
		return;
	}
	auto lambda = [wp = SafeWeakPtr<RigidBody>(std::static_pointer_cast<RigidBody>(shared_from_this())), velocity_]() {
		if (!wp)
			return;
		if (wp->is_kinematic)
			return;
		if (auto owner_obj = static_cast<SafeWeakPtr<Object>*>(wp->body->userData))
			if (wp->body->is<PxRigidDynamic>())
				static_cast<PxRigidDynamic*>(wp->body)->setLinearVelocity(velocity_);
		};
	SceneManager::GetCurrentScene()->AddFunctionAfterSimulation(lambda);
}

void RigidBody::ChangeToStatic()
{
	owner->GetScene()->DeleteActor(body);
	auto scene = owner->GetScene();
	auto p_scene = scene->GetPhysicsScene();
	body
		= PhysicsManager::GetPhysicsInstance()->createRigidStatic(physx::PxTransform(physx::PxIdentity));
	body->userData = new SafeWeakPtr<Object>(owner->shared_from_this());

	p_scene->addActor(*body);
}
