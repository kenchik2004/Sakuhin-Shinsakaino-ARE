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
		std::function on_jump_finish = [&]() {my_animator->Play("idle", true); };
		std::function on_salute_finish = [&]() {my_animator->Play("idle", true, 0, 0.8f); };
		std::function on_reload_finish = [&]() {my_animator->Play("aim", true); };
		my_animator->SetAnimationCallBack("reload", reload_call, 30, "reload_se");
		my_animator->SetAnimationCallBack("reload", on_reload_finish, 200, "on_reload_finish");
		my_animator->SetAnimationCallBack("jump", on_jump_finish, 130, "on_jump_finish");
		my_animator->SetAnimationCallBack("salute", on_salute_finish, 170, "on_salute_finish");
		return 0;
	}
	void SampleAnimationObject::Update()
	{
		if (Input::GetKeyDown(KeyCode::NumPad1))
			my_animator->PlayIfNoSame("idle", true);
		if (Input::GetKeyDown(KeyCode::NumPad2))
			my_animator->PlayIfNoSame("walk", true);
		if (Input::GetKeyDown(KeyCode::NumPad3))
			my_animator->PlayIfNoSame("run", true);
		if (Input::GetKeyDown(KeyCode::NumPad4))
			my_animator->Play("jump", false, 0, 0.2f, true);
		if (Input::GetKeyDown(KeyCode::NumPad5))
			my_animator->Play("salute", false);
		if (Input::GetKeyDown(KeyCode::NumPad6))
			my_animator->PlayIfNoSame("aim", true);
		if (Input::GetKeyDown(KeyCode::NumPad7))
			my_animator->Play("reload", false);
	}
	void SampleAnimationObject::DebugDraw()
	{
	}
	void SampleAnimationObject::Exit()
	{
	}
}