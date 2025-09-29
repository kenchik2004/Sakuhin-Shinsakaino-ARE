#include "precompile.h"
#include "AmeManager.h"
#include "Game/Objects/AmeGame/AmePlayer.h"
namespace AmeGame {
	Color ame_color = { 0,0.05f,0.2f,1 };
	int AmeManager::Init()
	{
		camera = SceneManager::Object::Get<AmePlayer>();
		return 0;
	}
	void AmeManager::Update()
	{
#if 1
		if (Input::GetKey(KeyCode::Alpha1))
			ame_color.r += Time::DeltaTime();
		if (Input::GetKey(KeyCode::Alpha2))
			ame_color.r -= Time::DeltaTime();
		if (Input::GetKey(KeyCode::Alpha3))
			ame_color.g += Time::DeltaTime();
		if (Input::GetKey(KeyCode::Alpha4))
			ame_color.g -= Time::DeltaTime();
		if (Input::GetKey(KeyCode::Alpha5))
			ame_color.b += Time::DeltaTime();
		if (Input::GetKey(KeyCode::Alpha6))
			ame_color.b -= Time::DeltaTime();
#endif
		if (GetScene()->loading_status != Scene::LOADING_STATUS::LOADED)
			return;
		for (auto& ame_ : ame)
		{
			ame_.position += ame_.dir * Time::RealDeltaTime() * 30.0f * (1.0f - ame_.reflect_timer);
			if (ame_.reflect_timer > 0)
				ame_.reflect_timer += Time::DeltaTime();
		}
	}
	void AmeManager::Draw()
	{
		if (GetScene()->loading_status != Scene::LOADING_STATUS::LOADED)
			return;

		SetDrawBlendMode(DX_BLENDMODE_ADD, 255);
		Vector3 cam_pos = camera->transform->position;
		for (auto& ame_ : ame)
		{
			RayCastInfo info;
			Ray ray{
				.position = ame_.position,
				.direction = ame_.dir,
				.length = ame_.reflect_timer > 0 ? (0.15f - ame_.reflect_timer) : 1,
			};

			ray.length = physx::PxClamp(ray.length, 0.0001f, 1.0f);
			if (ame_.position.y - cam_pos.y < 20 && ray.direction.magnitudeSquared()>0.0f)
				GetScene()->RayCast(ray, info);
			if (ame_.position.y < -5 || ame_.reflect_timer > 0.15f)
			{
				ame_.position = Random::Position(cam_pos + Vector3(-40, 20, -40), cam_pos + Vector3(40, 40, 40));
				ame_.reflect_timer = 0;
				ame_.dir = { 0,-1,0 };
				ame_.hit_position = { -100,-100,-100 };
			}
			if (info.hasBlock) {
				ame_.dir = ame_.dir - ame_.dir.dot(info.block.normal) * info.block.normal;
				ame_.dir.normalizeFast();
				if (!ame_.dir.isFinite() || ame_.dir.isZero())
					ame_.dir = info.block.normal;
				ame_.hit_position = info.block.position;
				ame_.position = info.block.position;
				ame_.reflect_timer += Time::DeltaTime();
			}
			if (ame_.hit_position != Vector3(-100, -100, -100)) {

				DrawCircle3D_XZ(cast(ame_.hit_position), ame_.reflect_timer * 0.5f, ame_color);
				DrawCircle3D_XZ(cast(ame_.hit_position), ame_.reflect_timer * 0.7f, ame_color);
			}
			DrawLine3D(cast(ame_.position), cast(ame_.position + ame_.dir * (ame_.reflect_timer > 0 ? (0.5f - ame_.reflect_timer) : 1.0f)), ame_color);
		}
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}