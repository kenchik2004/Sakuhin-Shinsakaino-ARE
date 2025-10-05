#include "precompile.h"
#include "AmePlayer.h"

#include "System/Components/ModelRenderer.h"
#include "System/Components/RigidBody.h"
#include "System/Objects/CameraObject.h"
#include "System/Components/CapsuleCollider.h"
#include "System/Components/SphereCollider.h"
#include "System/Components/Camera.h"
#include "System/Components/AudioListener.h"
#include "System/Components/AudioPlayer.h"
namespace AmeGame {

#ifdef _DEBUG
	bool fly_mode = false;
	float fly_input_tymelimit = 0.4f;
	float fly_input_delta = 0.0f;

#endif
	int AmePlayer::Init()
	{
		//プレイヤーの初期化(仮)
		auto rb = AddComponent<RigidBody>();
		rb->freeze_rotation = { true, true, true }; //カメラは回転しない
		auto col = AddComponent<CapsuleCollider>();
		col->radius = 0.5f;
		col->height = 1.45f;
		col->rotation = Quaternion(DEG2RAD(90), { 0,0,1 });
		col->position = { -1, 0, 0 }; //カプセルの上端をカメラの位置に合わせる
		//col->SetMaterial(Material::Concrete);
		transform->rotation = { 0, 0, 0 ,1 };
		auto foot_ = AddComponent<SphereCollider>();
		foot_->is_trigger = true;
		foot_->radius = 0.25f;
		foot_->position = { 0,-2.2f,0 };
		foot = foot_;

		auto camera_ = SceneManager::Object::Create<CameraObject>();

		camera_->transform->rotation = { 0, 0, 0 ,1 };
		transform->SetChild(camera_->transform);
		camera_->transform->local_position = { 0,-0.5f,0 };
		camera_->AddComponent<AudioListener>();
		auto audio_player = camera_->AddComponent<AudioPlayer>();
		audio_player->SetAudio("rain");
		audio_player->loop = true;
		audio_player->Play();
		transform->position = { 935,40,-317 };
		transform->AddRotation({ 0,90,0 });

		//スカイボックスをカメラに追従
		auto sky = SceneManager::Object::Create<Object>();
		auto model = sky->AddComponent<ModelRenderer>();
		model->SetModel("sky");
		model->cast_shadow = false;
		transform->SetChild(sky->transform);
		sky->transform->local_position = { 0,0,0 };
		sky->transform->scale = { 3,3,3 };
		return 0;
	}

	void AmePlayer::Update()
	{
		auto mouse_ = Input::GetMouseDelta();
		auto mouse_pos = Input::GetMousePosition();
		if (mouse_pos.x > SCREEN_W - 2)
			mouse_pos.x = 1;
		if (mouse_pos.x < 1)
			mouse_pos.x = SCREEN_W - 2;
		if (mouse_pos.y > SCREEN_H - 2)
			mouse_pos.y = 1;
		if (mouse_pos.y < 1)
			mouse_pos.y = SCREEN_H - 2;
#ifdef _DEBUG
		fly_input_delta -= Time::DeltaTime();
		if (Input::GetKeyDown(KeyCode::Space)) {
			if (fly_input_delta > 0) {
				fly_mode = !fly_mode;
				fly_input_delta = 0.0f;
				if (fly_mode) {
					auto rb = GetComponent<RigidBody>();
					rb->use_gravity = false;
					rb->velocity = { 0,0,0 };

				}
				else {
					GetComponent<RigidBody>()->use_gravity = true;
					is_jumping = true;
				}
			}

			else {
				fly_input_delta = fly_input_tymelimit;
			}
		}
		if(Input::GetKey(KeyCode::Space)&&fly_mode)
			transform->position += Vector3(0, Time::DeltaTime()*10, 0);
			
		if(Input::GetKey(KeyCode::LControl) && fly_mode)
			transform->position -= Vector3(0, Time::DeltaTime()*10, 0);
		
#endif

		Input::SetMousePosition(mouse_pos);
		transform->AddRotation(Quaternion(DEG2RAD(Time::DeltaTime() * 10 * mouse_.x), { 0, 1, 0 }));

		auto child = transform->GetChild(0).lock();
		if ((child->local_rotation * (Quaternion(DEG2RAD(Time::DeltaTime() * 10 * mouse_.y), { 1, 0, 0 }))).rotate({ 0,0,1 }).dot({ 0,0,1 }) > 0.2f)
			child->local_rotation *= (Quaternion(DEG2RAD(Time::DeltaTime() * 10 * mouse_.y), { 1, 0, 0 }));

		auto rb = GetComponent<RigidBody>();
		if (rb) {
			Vector3 mov = { 0,0,0 };
			if (Input::GetKey(KeyCode::W)) {
				mov += transform->AxisZ();
			}
			if (Input::GetKey(KeyCode::S)) {
				mov -= transform->AxisZ();
			}
			if (Input::GetKey(KeyCode::A)) {
				mov -= transform->AxisX();
			}
			if (Input::GetKey(KeyCode::D)) {
				mov += transform->AxisX();
			}
			if (Input::GetKeyDown(KeyCode::Space) && !is_jumping) {
				if (rb) {
					rb->AddForce(Vector3(0, 6, 0), ForceMode::Impulse);
					is_jumping = true;
				}
			}
			mov.normalize();
			mov *= 5.0f;
			if (Input::GetKey(KeyCode::LShift))
			{
				mov *= 1.3f; //シフトキーで移動速度を2倍にする
			}
			mov.y = rb->velocity.y;
			rb->velocity = mov;
		}
	}
	void AmePlayer::OnTriggerEnter(const HitInfo& hit_info)
	{
		if (hit_info.collision == foot.lock() && hit_info.hit_collision->owner.lock().raw_shared() != shared_from_this())
			is_jumping = false;
	}
}
