#pragma once
#include <System/Component.h>
struct LockFlag {
	bool x = false;
	bool y = false;
	bool z = false;
};
enum ForceMode {		//力を加えるモード
	Force = physx::PxForceMode::eFORCE,						//重さを加味した単発的な力の変化(N/s^2)
	Impulse = physx::PxForceMode::eIMPULSE,					//重さを加味した継続的な運動量の変化(N/s)
	VelocityCange = physx::PxForceMode::eVELOCITY_CHANGE,	//重さを無視した速度の変化(m/s)
	Acceleration = physx::PxForceMode::eACCELERATION,		//重さを無視した加速度の変化(m/s^2)
};
USING_PTR(RigidBody);
class RigidBody :public Component
{
public:
	USING_SUPER(RigidBody);
	void Construct() override;
	int Init() override;
	void PrePhysics() override;
	void PostPhysics() override;
	void Update() override;
	void DebugDraw() override;
	void Exit() override;
	void AddForce(Vector3 force, ForceMode force_mode = ForceMode::Force);
	void AddTorque(Vector3 torque, ForceMode force_mode = ForceMode::Force);
	void AddForceAtPosition(Vector3 force, Vector3 world_position, ForceMode force_mode = ForceMode::Force);
	void SetMassCenter(Vector3 center);
	void SetVelocity(Vector3 velocity_);
	Vector3 velocity = { 0,0,0 };
	Vector3 angular_velocity = { 0,0,0 };
	float mass = 1.0f;
	physx::PxRigidActor* GetBody() { return body; }
	LockFlag freeze_position = { 0,0,0 };
	LockFlag freeze_rotation = { 0,0,0 };
	bool use_gravity = true;
	bool is_kinematic = false;
	void ChangeToStatic();

private:
	physx::PxRigidActor* body = nullptr;
	Vector3 pos = { 0,0,0 };
	Quaternion rot = Quaternion(physx::PxIdentity);
};

