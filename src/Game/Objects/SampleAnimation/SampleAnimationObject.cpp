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
		if (Input::GetKeyDown(KeyCode::NumPad1))
			my_animator->PlayIfNoSame("idle", true);
		if (Input::GetKeyDown(KeyCode::NumPad2))
			my_animator->PlayIfNoSame("walk", true);
		if (Input::GetKeyDown(KeyCode::NumPad3))
			my_animator->PlayIfNoSame("run", true);
		if (Input::GetKeyDown(KeyCode::NumPad4))
			my_animator->Play("jump", false, (my_animator->anim_speed > 0) ? 0 : (130 / 60.0f), 0.2f, true);
		if (Input::GetKeyDown(KeyCode::NumPad5))
			my_animator->Play("salute", false, (my_animator->anim_speed > 0) ? 0 : (170 / 60.0f), 0.2f);
		if (Input::GetKeyDown(KeyCode::NumPad6))
			my_animator->PlayIfNoSame("aim", true);
		if (Input::GetKeyDown(KeyCode::NumPad7))
			my_animator->Play("reload", false, (my_animator->anim_speed > 0) ? 0 : (200 / 60.0f));

		if (Input::GetKeyDown(KeyCode::Return)) {
			bool normal_play = (my_animator->anim_speed *= -1.0f) > 0;
			//�t�Đ��̏ꍇ�A�A�j���[�V�����̍Ō�̃t���[���Ŏ��s����R�[���o�b�N�́A
			//�ŏ��̃t���[���Ɉړ������Ȃ��Ƃ����Ȃ�
			std::function on_jump_finish = [=]() {my_animator->Play("idle", true, 0.0f, 1.0f); };
			std::function on_salute_finish = [=]() {my_animator->Play("idle", true, 5.0f, 0.2f); };
			std::function on_reload_finish = [=]() {my_animator->Play("aim", true); };
			my_animator->SetAnimationCallBack("reload", on_reload_finish, normal_play ? 200 : 0, "on_reload_finish");
			my_animator->SetAnimationCallBack("jump", on_jump_finish, normal_play ? 130 : 0, "on_jump_finish");
			my_animator->SetAnimationCallBack("salute", on_salute_finish, normal_play ? 170 : 0, "on_salute_finish");
		}
		if (Input::GetKeyDown(KeyCode::Space)) {
			int model_texture_handle = MV1GetMaterialNormalMapTexture(my_model->GetModelHandle(), 0);
			int x, y;
			GetGraphTextureSize(model_texture_handle, &x, &y);

			SaveDrawValidGraph(model_texture_handle, 0, x, 0, y, "data/player/model_texture.png", DX_IMAGESAVETYPE_PNG);
		}

	}

	void SampleAnimationObject::Exit()
	{
	}
}