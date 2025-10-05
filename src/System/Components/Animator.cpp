#include "precompile.h"
#include "Animator.h"
#include "ModelRenderer.h"
#include "System/Components/MeshCollider.h"
#include "System/ModelManager.h"


void Animator::Construct()
{
	old_anims.fill(nullptr);
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

void Animator::SetAnimationCallBack(std::string_view anim_name, std::function<void()>& call_back, float execute_frame, std::string_view method_name)
{
	for (auto& ite : animation) {
		if (ite->name == anim_name) {
			ite->SetCallBack(call_back, execute_frame, method_name);
			return;
		}
	}
}

void Animator::ResetAnimationCallBack(std::string_view anim_name, std::string_view method_name)
{
	for (auto& ite : animation) {
		if (ite->name == anim_name) {
			ite->ResetCallBack(method_name);
			return;
		}
	}
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

	float blend_rate_total = 0.0f;
	anim_blend_time = max(FLT_EPSILON, anim_blend_time);	//0割り防止
	//一旦全アニメーションの時間を進める
	if (current_anim) {
		current_anim->Update(anim_speed);
		MV1SetAttachAnimTime(model->GetModelHandle(), current_anim->attached_index, current_anim->current_time);
		if (anim_loop && !IsPlaying()) {
			current_anim->current_time = current_anim->current_time > current_anim->total_time ? 0 : current_anim->total_time;
			current_anim->InitCallBacks();
		}

		current_anim->blend_rate += (1.0f / anim_blend_time) * Time::RealDeltaTime();
		current_anim->blend_rate = std::clamp(current_anim->blend_rate, 0.0f, 1.0f);
		blend_rate_total += current_anim->blend_rate;

		//ヒップのY座標を固定する
		if (freeze_hip_y)
		{
			//ヒップのY座標を固定する
			auto mat = MV1GetFrameBaseLocalMatrix(model->GetModelHandle(), 0);
			//現在再生中のアニメーションの、リアルタイムのフレームの行列を取得する
			auto anim_mat = MV1GetAttachAnimFrameLocalMatrix(model->GetModelHandle(), current_anim->attached_index, 0);

			anim_mat.m[3][1] = mat.m[3][1];//Y座標だけ書き換え、あとはアニメーションの行列を使う
			//ここで大事なのは、回転やxz座標はアニメーションの行列を使うこと
			//全部書き換えてしまうと、腰骨が完全に固定され、ゴキブリみたいな動きになってしまう

			MV1SetFrameUserLocalMatrix(model->GetModelHandle(), 0, anim_mat);
		}
	}
	//古いアニメーションも更新する
	for (auto& old_anim : old_anims) {
		if (old_anim) {
			old_anim->Update(anim_speed);
			//アニメーションの時間を進める
			//ループしているかどうかはこの段階では知ったこっちゃないので、とりあえず進めるだけ進める
			MV1SetAttachAnimTime(model->GetModelHandle(), old_anim->attached_index, old_anim->current_time);
			old_anim->blend_rate -= (1.0f / anim_blend_time) * Time::RealDeltaTime();
			old_anim->blend_rate = std::clamp(old_anim->blend_rate, 0.0f, 1.0f);
			blend_rate_total += old_anim->blend_rate;
		}
	}
	blend_rate_total = max(FLT_EPSILON, blend_rate_total);	//0割り防止
	//ブレンド率を設定する
	if (current_anim) {
		MV1SetAttachAnimBlendRate(model->GetModelHandle(), current_anim->attached_index, current_anim->blend_rate / blend_rate_total);
	}
	for (auto& old_anim : old_anims) {
		if (old_anim) {
			MV1SetAttachAnimBlendRate(model->GetModelHandle(), old_anim->attached_index, old_anim->blend_rate / blend_rate_total);
			//ブレンド率が0になったらアタッチを外して、アニメーションを消す
			if (old_anim->blend_rate <= 0.0f) {

				MV1DetachAnim(model->GetModelHandle(), old_anim->attached_index);
				old_anim->attached_index = -1;
				old_anim->current_time = 0;
				old_anim.reset();
			}
		}
	}


}


void Animator::Exit()
{
	if (IsPlaying())
		Stop();
	model.reset();
	current_anim.reset();
	old_anims.fill(nullptr);
	animation.clear();
}


void Animator::Play(std::string_view name, bool loop, float start_time, float blend_time, bool freeze_y)
{
	anim_blend_time = blend_time;
	//モデルコンポーネントがない、またはモデルがロードされていなかったら処理を抜ける
	if (!model)
		return;
	if (model->GetModelHandle() < 0)
		return;

	// 名前からアニメーションを探す
	SafeSharedPtr<Animation> select = nullptr;
	for (auto& ite : animation) {
		if (ite->name == name) {
			select = ite;
			break;
		}
	}
	//もしモデルコンポーネントにアニメーションがセットされていなかったら、処理を抜ける
	if (!select)
		return;

	//ヒップのY座標を固定するかどうか
	freeze_hip_y = freeze_y;
	if (!freeze_hip_y)
	{
		//ヒップのY座標を固定しない場合は、ユーザーローカル行列をリセットしておく
		MV1ResetFrameUserLocalMatrix(model->GetModelHandle(), 0);
	}
	//再生中のアニメーションがあるなら、そちらを古いアニメーションに移動させ、
	//新しいアニメーションを再生中のものとしてセットする

	if (current_anim) {

		//同じ(もしくは古いものの中に登録されている)アニメーションを再生しようとしているなら、最初からリスタート
		{
			auto in_old = std::find(old_anims.begin(), old_anims.end(), select);
			//古いアニメーションの中にあった場合は、現在のアニメーションと入れ替える
			if (in_old != old_anims.end())
				std::swap(*in_old, current_anim);
		}
		if (current_anim == select)
		{
			if (start_time == CUR_ANIMTIME_MAX)
				start_time = select->total_time / 60.0f;
			current_anim->current_time = start_time * 60;
			current_anim->blend_rate = 0.0f;
			current_anim->InitCallBacks();
			anim_loop = loop;
			return;
		}
		//アタッチに失敗した場合は、現在のアニメーションを変更しないので、先にアタッチを試みる
		if (select->attached_index >= 0) {
			//すでにアタッチされている場合は、一旦外す
			MV1DetachAnim(model->GetModelHandle(), select->attached_index);
			select->attached_index = -1;
		}
		select->attached_index = MV1AttachAnim(model->GetModelHandle(), select->index, select->handle, false);


		//古いアニメーションを一つずつ後ろにずらしていき、一番古いものを消す
		//現在再生中のものは一番新しいものとして突っ込む
		{
			auto& oldest = old_anims.back();
			//一番古いアニメーションを消す
			{
				if (oldest) {
					int result = MV1DetachAnim(model->GetModelHandle(), oldest->attached_index);
					if (result < 0) {
						int a = 0;
						a++;
					}
					oldest->attached_index = -1;
					oldest->current_time = 0;
					oldest.reset();
				}
			}
			//古いものから一つずつ後ろにずらしていく
			for (int i = old_anims.size() - 2; i >= 0; i--) {
				old_anims[i + 1] = old_anims[i];
			}
			//現在のアニメーション古いやつの中でも一番新しいものとして突っ込む
			old_anims[0] = current_anim;
		}

		//指定されたアニメーションを再生中のものとしてセットする
		{
			anim_loop = loop;
			if (start_time == CUR_ANIMTIME_MAX)
				start_time = select->total_time / 60.0f;
			select->current_time = start_time * 60;
			select->InitCallBacks();
			select->blend_rate = 0.0f;
			current_anim = select;
			anim_loop = loop;
		}
		return;
	}

	//現在のアニメーションがない場合は、普通にアタッチして再生する
	if (select->attached_index >= 0) {
		//すでにアタッチされている場合は、一旦外す
		MV1DetachAnim(model->GetModelHandle(), select->attached_index);
		select->attached_index = -1;
	}
	select->attached_index = MV1AttachAnim(model->GetModelHandle(), select->index, select->handle, false);
	if (select->attached_index >= 0) {
		anim_loop = loop;
		current_anim = select;
		current_anim->InitCallBacks();
		if (start_time == CUR_ANIMTIME_MAX)
			start_time = select->total_time / 60.0f;
		current_anim->current_time = start_time * 60;
		current_anim->blend_rate = 0.0f;
	}
}

void Animator::Stop()
{
	if (!model || !current_anim)
		return;
	if (model->GetModelHandle() < 0)
		return;

	//古いアニメーションを一つずつ後ろにずらしていき、一番古いものを消す
	//現在再生中のものは一番新しいものとして突っ込む
	{
		//一番古いアニメーションを消す
		{
			auto& oldest = old_anims.back();
			if (oldest) {

				MV1DetachAnim(model->GetModelHandle(), oldest->attached_index);
				oldest->attached_index = -1;
				oldest->current_time = 0;
				oldest.reset();
			}
		}
		//古いものから一つずつ後ろにずらしていく
		for (auto old = old_anims.end() - 2; old > old_anims.begin(); old--) {
			*(old + 1) = *old;
		}
		//現在のアニメーション古いやつの中でも一番新しいものとして突っ込む
		old_anims[0] = current_anim;
	}

	anim_loop = false;
	current_anim->InitCallBacks();
	current_anim.reset();

}

void Animator::PlayIfNoSame(std::string_view name, bool loop, float start_time, float blend_time, bool freeze_y)
{
	//再生中のアニメーションがなければ、普通に再生する
	if (!current_anim) {
		Play(name, loop, start_time, blend_time, freeze_y);
		return;
	}
	//あったとしても再生中のアニメーションと同じ名前なら、何もしない
	if (current_anim->name != name) {
		Play(name, loop, start_time, blend_time, freeze_y);
	}

}

std::string_view Animator::GetCurrentAnimName()
{
	//再生中のアニメーションがあれば、その名前を返す
	if (current_anim && IsPlaying())
		return current_anim->name;
	//なければ空文字を返す
	return "";
}

bool Animator::IsPlaying()
{
	//アニメーションがセットされていなかったら、再生していない
	if (!current_anim)
		return false;
	//アニメーションの時間が総時間を超えていないかつ、0以上であるなら再生中とみなす
	// (再生速度をマイナスにして逆再生もできるように)
	return (current_anim->current_time < current_anim->total_time && current_anim->current_time >= 0);
}
