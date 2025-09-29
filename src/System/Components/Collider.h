#pragma once
#include "System/Component.h"

USING_PTR(RigidBody);
USING_PTR(Collider);
class Collider :
	public Component
{
public:
	USING_SUPER(Collider);
	bool is_trigger = false;
	void Construct() override;
	int Init() override;
	void Exit() override;
	Vector3 position = { 0,0,0 };
	Quaternion rotation = { 0,0,0,1 };
	RigidBodyP GetRigidBody() { return rigidbody.lock(); }
	void AttachToModel(int attach_index);
	void SetMaterial(physx::PxMaterial* new_mat);
	enum Layer :physx::PxU32 {
		Default = 1,
		Wepon = 1 << 1,
		Enemy = 1 << 2,
		All = UINT32_MAX,
	};
	Layer hit_group = All;
	Layer collision_group = Default;
	void SetLayer(Layer layer);
protected:
	bool attach_to_model = false;
	int model_attach_index = -1;
	RigidBodyWP rigidbody;
	physx::PxShape* shape;

	physx::PxTransform MakeCollisionTransform();

};

