#include "precompile.h"
#include "SampleAnimationObject.h"

#include "System/Components/ModelRenderer.h"
#include "System/Components/Animator.h"

namespace SampleAnimation
{
	int SampleAnimationObject::Init()
	{
		my_model = AddComponent<ModelRenderer>();
		my_model->SetModel("player_model");
		my_animator = AddComponent<Animator>();
		my_animator->SetAnimation("idle", 1);
		my_animator->SetAnimation("walk", 1);
		my_animator->SetAnimation("run", 1);
		my_animator->SetAnimation("jump", 0);
		my_animator->SetAnimation("salute", 0);
		my_animator->SetAnimation("aim", 1);
		my_animator->SetAnimation("reload", 1);


		my_animator->anim_blend_time = 0.2f;
		transform->scale = Vector3(0.05f, 0.05f, 0.05f);
		//my_animator->Play("idle", true, 0.0f);
		auto reload_se = AudioManager::CloneByName("reload_se");
		std::function reload_call = [reload_se]() {
			reload_se->PlayOneShot();
			};
		std::function on_jump_finish = [=]() {my_animator->Play("idle", true, 0.0f, 1.0f); };
		std::function on_salute_finish = [=]() {my_animator->Play("idle", true, 5.0f, 0.2f); };
		std::function on_reload_finish = [=]() {my_animator->Play("aim", true); };
		my_animator->SetAnimationCallBack("reload", reload_call, 30, "reload_se");
		my_animator->SetAnimationCallBack("reload", on_reload_finish, 200, "on_reload_finish");
		my_animator->SetAnimationCallBack("jump", on_jump_finish, 130, "on_jump_finish");
		my_animator->SetAnimationCallBack("salute", on_salute_finish, 170, "on_salute_finish");
		return 0;
	}
	void SampleAnimationObject::Update()
	{
		if (manipulate_mode)
			return;
		if (Input::GetKeyDown(KeyCode::Alpha1))
			my_animator->PlayIfNoSame("idle", true);
		if (Input::GetKeyDown(KeyCode::Alpha2))
			my_animator->PlayIfNoSame("walk", true);
		if (Input::GetKeyDown(KeyCode::Alpha3))
			my_animator->PlayIfNoSame("run", true);
		if (Input::GetKeyDown(KeyCode::Alpha4))
			my_animator->Play("jump", false, (my_animator->anim_speed > 0) ? 0 : (130 / 60.0f), 0.2f, true);
		if (Input::GetKeyDown(KeyCode::Alpha5))
			my_animator->Play("salute", false, (my_animator->anim_speed > 0) ? 0 : (170 / 60.0f), 0.2f);
		if (Input::GetKeyDown(KeyCode::Alpha6))
			my_animator->PlayIfNoSame("aim", true);
		if (Input::GetKeyDown(KeyCode::Alpha7))
			my_animator->Play("reload", false, (my_animator->anim_speed > 0) ? 0 : (200 / 60.0f));

#if 0
		if (Input::GetKeyDown(KeyCode::Return)) {
			bool normal_play = (my_animator->anim_speed *= -1.0f) > 0;
			//逆再生の場合、アニメーションの最後のフレームで実行するコールバックは、
			//最初のフレームに移動させないといけない
			std::function on_jump_finish = [=]() {my_animator->Play("idle", true, 0.0f, 1.0f); };
			std::function on_salute_finish = [=]() {my_animator->Play("idle", true, 5.0f, 0.2f); };
			std::function on_reload_finish = [=]() {my_animator->Play("aim", true); };
			my_animator->SetAnimationCallBack("reload", on_reload_finish, normal_play ? 200 : 0, "on_reload_finish");
			my_animator->SetAnimationCallBack("jump", on_jump_finish, normal_play ? 130 : 0, "on_jump_finish");
			my_animator->SetAnimationCallBack("salute", on_salute_finish, normal_play ? 170 : 0, "on_salute_finish");
		}
#endif


	}

	void SampleAnimationObject::Exit()
	{
	}
	void SampleAnimationObject::ManipulateAsAnotherPlayer(unsigned int anim_state)
	{
		switch (anim_state)
		{
		case 0:
			my_animator->PlayIfNoSame("idle", true);
			break;
		case 1:
			my_animator->PlayIfNoSame("walk", true);
			break;
		case 2:
			my_animator->PlayIfNoSame("run", true);
			break;
		case 3:
			my_animator->PlayIfNoSame("jump", false, 0, 0.2f, true);
			break;
		case 4:
			my_animator->PlayIfNoSame("salute", false, 0, 0.2f);
			break;
		case 5:
			my_animator->PlayIfNoSame("aim", true);
			break;
		case 6:
			my_animator->PlayIfNoSame("reload", false, 0);
			break;
		default:
			break;
		}
	}
	unsigned int SampleAnimationObject::GetCurrentAnimState()
	{
		if (my_animator->GetCurrentAnimName() == "idle")
			return 0;
		if (my_animator->GetCurrentAnimName() == "walk")
			return 1;
		if (my_animator->GetCurrentAnimName() == "run")
			return 2;
		if (my_animator->GetCurrentAnimName() == "jump")
			return 3;
		if (my_animator->GetCurrentAnimName() == "salute")
			return 4;
		if (my_animator->GetCurrentAnimName() == "aim")
			return 5;
		if (my_animator->GetCurrentAnimName() == "reload")
			return 6;

	}
	void SampleAnimationObject::LateDraw()
	{
		VECTOR pos=cast(transform->position);
		pos.y += 10;
		pos = DxLib::ConvWorldPosToScreenPos(pos);
		if (pos.z > 0)
			DrawFormatString(pos.x, pos.y, Color::BLUE, name.c_str());
	}
}