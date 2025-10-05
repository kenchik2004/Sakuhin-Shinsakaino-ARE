#pragma once
#include "System/Component.h"
#include "System/ModelManager.h"


#define CUR_ANIMTIME_MAX FLT_MAX

USING_PTR(Animator);
class Animator :
	public Component
{
public:
	USING_SUPER(Animator);
	void Construct() override;
	int Init() override;
	void Update() override;
	void Exit() override;
	//----------------------------------------------------------------------------------
	//アニメーションコンポーネントが管理するべきもの
	float anim_speed = 1.0f;
	float anim_blend_time = 0.5f;
	bool anim_loop = false;
	bool freeze_hip_y = false;
	SafeWeakPtr<ModelRenderer> model;
	std::vector<SafeSharedPtr<Animation>> animation;
	SafeSharedPtr<Animation> current_anim = nullptr;

	std::array<SafeSharedPtr<Animation>, 8> old_anims;	//!< ブレンド用に古いアニメーションを2つまで保持しておく


	void Play(std::string_view name, bool loop = false, float start_time = 0.0f, float blend_time = 0.2f, bool freeze_y = false);
	void Stop();
	void PlayIfNoSame(std::string_view name, bool loop = false, float start_time = 0.0f, float blend_time = 0.2f, bool freeze_y = false);
	std::string_view GetCurrentAnimName();
	bool IsPlaying();
	void SetAnimation(std::string_view name, int index = 0, std::string_view new_name = "");
	void SetAnimation(SafeSharedPtr<Animation> anim);
	void SetAnimationCallBack(std::string_view anim_name, std::function<void()>& call_back, float execute_frame, std::string_view method_name);
	void ResetAnimationCallBack(std::string_view anim_name, std::string_view method_name);
	static void Load(std::string_view path, std::string_view name);

	//----------------------------------------------------------------------------------

};

