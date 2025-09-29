#include "precompile.h"
#include "DiceBase.h"

#include "System/Components/ModelRenderer.h"
#include "System/Components/RigidBody.h"
#include "System/Components/ConvexMeshCollider.h"

namespace RLyeh {

	int DiceBase::Init()
	{
		auto mod = AddComponent<ModelRenderer>();
		SetPriority(10);
		//transform->scale = { 0.5f,0.5f,0.5f };
		mod->SetModel(model_name);
		MV1SetTextureGraphHandle(mod->GetModelHandle(), 0, handle, true);
		MV1SetOpacityRate(mod->GetModelHandle(), 0.7f);
		MV1SetZBufferCmpType(mod->GetModelHandle(), DX_CMP_ALWAYS);
		//mod->scale = { 0.01f,0.01f,0.01f };
		transform->position = Vector3((GetRand(100) - 50) * 0.1f, 1.6f, (GetRand(100) - 50) * 0.1f);
		transform->rotation = Quaternion(DEG2RAD(GetRand(360)), Vector3((GetRand(10) - 5) * 0.1f, (GetRand(10) - 5) * 0.1f, (GetRand(10) - 5) * 0.1f).getNormalized());


		auto rb = AddComponent<RigidBody>();
		//static_cast<physx::PxRigidDynamic*>(rb->GetBody())->setAngularDamping(2.0f);
		rb->AddTorque(Vector3((float)(GetRand(800) - 400), (float)(GetRand(800) - 400), (float)(GetRand(800) - 400)), ForceMode::VelocityCange);
		rb->AddForce(Vector3((float)GetRand(50) - 25, (float)-GetRand(20), (float)GetRand(50) - 25), ForceMode::VelocityCange);
		auto col = AddComponent<ConvexMeshCollider>();
		rb->mass = 5;
		return 0;
	}

	void DiceBase::FetchResult()
	{
		float dot = transform->rotation.rotate(dice_vectors[0].getNormalized()).dot(Vector3(0, 1, 0));
		float dot2 = 0;
		for (int i = 0; i < dice_vectors.size(); i++) {
			dot2 = transform->rotation.rotate(dice_vectors[i].getNormalized()).dot(Vector3(0, 1, 0));
			if (dot2 > dot) {
				selected_number = i;
				dot = dot2;
			}
		}
		int a = 0;
	}
	void DiceBase::DebugDraw()
	{
		for (int i = 0; i < dice_vectors.size(); i++) {
			DrawLine3D(cast(transform->position), cast(transform->position + transform->rotation.rotate(dice_vectors[i]) * 0.5f), Color::WHITE);
		}
	}

	void DiceBase::LateDebugDraw()
	{
		auto obj_pos = ConvWorldPosToScreenPos(cast(transform->position + Vector3(0, 0.5f, 0)));;
		auto pos = ConvWorldPosToScreenPos(cast(transform->position + Vector3(1.5f, 1, 0)));
		if (pos.z < 1) {
			DrawLineAA(obj_pos.x, obj_pos.y, pos.x, pos.y, Color::WHITE);
			DrawFormatString(pos.x, pos.y, Color::WHITE, " NUMBER:%d", selected_number + 1);
		}
	}

}