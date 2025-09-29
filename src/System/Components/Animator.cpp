#include "precompile.h"
#include "Animator.h"
#include "ModelRenderer.h"
#include "System/Components/MeshCollider.h"
#include "System/ModelManager.h"


void Animator::Construct()
{
	status.status_bit.on(CompStat::STATUS::SINGLE);
}

int Animator::Init()
{
	if (auto m = owner.lock()->GetComponent<ModelRenderer>())
		model = m;
	else
		RemoveThisComponent();
	return 0;
}


void Animator::SetAnimation(std::string_view name, int index, std::string_view new_name)
{

	auto data = ModelManager::CloneAnimByName(name, index, new_name);
	if (!data)
		return;
	animation.push_back(data);
}

void Animator::SetAnimation(SafeSharedPtr<Animation> anim)
{
	if (!anim)
		return;
	animation.push_back(anim);
}

void Animator::Load(std::string_view path, std::string_view name)
{
	ModelManager::LoadAsAnimation(path, name);
}

void Animator::Update()
{
	if (!model)
		return;
	if (model->GetModelHandle() < 0)
		return;
	anim_time += Time::RealDeltaTime() * 60 * anim_speed;
	if (current_index == -1)
		current_anim = nullptr;
	if (current_anim) {
		MV1SetAttachAnimTime(model->GetModelHandle(), current_index, anim_time);
		current_anim->Update(anim_time);
		if (anim_loop && !IsPlaying()) {
			anim_time = anim_time > current_anim->total_time ? 0 : current_anim->total_time;
			current_anim->InitCallBacks();
		}
	}
}


void Animator::Exit()
{
	if (IsPlaying())
		Stop();
	model.reset();
	current_anim.reset();
	animation.clear();
}


void Animator::Play(std::string_view name, bool loop, float start_time)
{
	if (!model)
		return;
	if (model->GetModelHandle() < 0)
		return;
	SafeSharedPtr<Animation> select = nullptr;
	for (auto& ite : animation) {
		if (ite->name == name) {
			select = ite;
			break;
		}
	}
	if (!select)
		return;
	if (current_anim) {
		MV1DetachAnim(model->GetModelHandle(), current_index);
		current_index = -1;
		current_index = MV1AttachAnim(model->GetModelHandle(), select->index, select->handle, false);
		current_anim->InitCallBacks();
		if (current_index >= 0) {
			current_anim = select;
			current_anim->InitCallBacks();
			anim_loop = loop;
			if (start_time == CUR_ANIMTIME_MAX)
				start_time = select->total_time / 60.0f;
			anim_time = start_time * 60;
		}
		return;
	}
	current_index = MV1AttachAnim(model->GetModelHandle(), select->index, select->handle, false);
	if (current_index >= 0) {
		anim_loop = loop;
		current_anim = select;
		current_anim->InitCallBacks();
		if (start_time == CUR_ANIMTIME_MAX)
			start_time = select->total_time / 60.0f;
		anim_time = start_time * 60;
	}
}

void Animator::Stop()
{
	if (!model || !current_anim)
		return;
	if (model->GetModelHandle() < 0)
		return;
	MV1DetachAnim(model->GetModelHandle(), current_index);
	current_index = -1;
	anim_loop = false;
	current_anim->InitCallBacks();
	current_anim.reset();

}

void Animator::PlayIfNoSame(std::string_view name, bool loop, float start_time)
{
	if (!current_anim) {
		Play(name, loop, start_time);
		return;
	}

	if (current_anim->name != name) {
		Play(name, loop, start_time);
	}

}

std::string_view Animator::GetCurrentAnimName()
{
	if (current_anim && IsPlaying())
		return current_anim->name;
	return "";
}

bool Animator::IsPlaying()
{
	if (!current_anim)
		return false;
	return (anim_time < current_anim->total_time && anim_time >= 0);
}
