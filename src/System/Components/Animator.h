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
	float anim_time = 0;
	float anim_speed = 1.0f;
	bool anim_loop = false;
	SafeWeakPtr<ModelRenderer> model;
	std::vector<SafeSharedPtr<Animation>> animation;
	SafeSharedPtr<Animation> current_anim = nullptr;
	int current_index = -1;


	void Play(std::string_view name, bool loop = false, float start_time = 0.0f);
	void Stop();
	void PlayIfNoSame(std::string_view name, bool loop = false, float start_time = 0.0f);
	std::string_view GetCurrentAnimName();
	bool IsPlaying();
	void SetAnimation(std::string_view name, int index = 0, std::string_view new_name = "");
	void SetAnimation(SafeSharedPtr<Animation> anim);
	static void Load(std::string_view path, std::string_view name);

	//----------------------------------------------------------------------------------

};

