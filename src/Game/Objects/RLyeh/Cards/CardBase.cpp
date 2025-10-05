#include "precompile.h"
#include "CardBase.h"
#include "System/Components/Camera.h"

namespace RLyeh {
	SafeSharedPtr<Texture> mytex = nullptr;
	int count = 0;
	int CardBase::Init()
	{

		SetPriority(19);
		model = AddComponent<ModelRenderer>();
		//TextureManager::Load(reinterpret_cast<const char*>(u8"data/カード/CardTexture.png"), "my_tex");
		//auto texture = SceneManager::GetCurrentScene()->GetCurrentCameraRef();
		if (!mytex) {
			TextureManager::Load(reinterpret_cast<const char*>(u8"data/カード/CardTexture_Temp.png"), "card_temp");
			mytex = TextureManager::CloneByName("card_temp");
		}
		if (!my_texture)
			my_texture = mytex;
		model->SetModel("card");


		SetTexture(my_texture);
		//MV1SetOpacityRate(model->GetModelHandle(), 0.1f);
		transform->position = Random::Position({ 3,0,3 }, { -3,0,-3 });
		start_pos = transform->position;
		return 0;
	}

	void CardBase::Update()
	{
		count++;
		auto cam = GetScene()->GetCurrentCameraRef()->owner;
		Vector3 target = Vector3(0, 4, 0);
		Quaternion target_rot = { 0,0,0,1 };
		if (cam) {
			target = cam->transform->position + cam->transform->AxisZ() * 4.5f;
			target_rot = cam->transform->rotation * Quaternion(DEG2RAD(-90), { 1,0,0 });
		}
		switch (mode)
		{
		case 0:
			delta = 0;
			if (Input::GetKeyDown(KeyCode::Return))
				mode++;
			break;
		case 1:
			transform->AddRotation({ 0,0, Time::DeltaTime() * 120 });
			transform->position = Lerp(start_pos, target, delta);
			delta += Time::DeltaTime();
			if (delta > 3.0f) {
				transform->position = target;
				mode++;
				delta = 0;
			}

			if (0 && (target - transform->position).magnitudeSquared() <= 0.05f) {
				transform->position = target;
				mode++;
				delta = 0;
			}
			break;
		case 2:
			delta += Time::UnscaledDeltaTime();
			{
				Quaternion qx(DEG2RAD(-15), Vector3(1, 0, 0));
				Quaternion qy(DEG2RAD(10), Vector3(0, 1, 0));
				Quaternion qz(DEG2RAD(10), Vector3(0, 0, 1));
				transform->rotation = Slerp(transform->rotation, target_rot * qx * qy * qz, 0.1f * Time::DeltaTime() * 60);
			}
			transform->position = target;
			if (delta >= 1.0f) {
				transform->ResetParent();
				delta = 0;
				mode++;
			}
			break;
		case 3:
			transform->rotation = Slerp(transform->rotation, target_rot, delta);
			transform->position = target;
			delta += Time::UnscaledDeltaTime();
			if (delta > 0.7f) {
				//transform->rotation = target_rot;
				mode++;
				delta = 0;
			}
			break;
		case 4:
			transform->position = Lerp(target, { 0,0.5f,0 }, delta);
			delta += Time::UnscaledDeltaTime();
			transform->rotation = Slerp(transform->rotation, { 0,0,0,1 }, delta);
			if (delta > 1) {

				mode++;
			}
			break;
		case 5:
			transform->position = { 0,0.5f,0 };
			if (Input::GetKeyDown(KeyCode::Return)) {
				transform->position = Random::Position({ -5,0.1f,-5 }, { 5,0.1f,5 });
				start_pos = transform->position;
				mode = 0;
			}
			break;
		default:
			break;
		}

		if (Input::GetKeyDown(KeyCode::T))
			SetPriority(GetRand(50));
		//if (Input::GetKeyDown(KeyCode::Return))
		//	++mode < 5 ? 0 : mode = 0;
	}

	void CardBase::LateUpdate()
	{
		count = 0;
	}

	void CardBase::Exit()
	{
		mytex.reset();
	}

	void CardBase::Draw()
	{
	}
	void CardBase::LateDraw()
	{
		//DrawFormatString(100, 48, Color::CYAN, "%f", delta);
	}

	void CardBase::SetTexture(SafeSharedPtr<Texture> tex)
	{
		model->OverrideTexture(tex, 0);
	}

}