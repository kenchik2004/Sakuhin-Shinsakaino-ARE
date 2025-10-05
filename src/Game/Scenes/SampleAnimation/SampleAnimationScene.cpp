#include "precompile.h"
#include "SampleAnimationScene.h"
#include "Game/Objects/SampleAnimation/SampleAnimationObject.h"
#include "System/Objects/CameraObject.h"

namespace SampleAnimation {


	bool SampleAnimationScene::CheckForLoading()
	{
		if (loading_status == LOADING_STATUS::LOADING) {
			loading_status = (ModelManager::GetLoadingCount() == 0 && AudioManager::GetLoadingCount() == 0) ? LOADING_STATUS::LOADED : LOADING_STATUS::LOADING;
			if (loading_status == LOADING_STATUS::LOADED) {
				Init();
				return true;
			}
			return false;
		}
		return true;
	}
	void SampleAnimationScene::Load()
	{
		ModelManager::LoadAsModel("data/player/model.mv1", "player_model");
		ModelManager::LoadAsAnimation("data/player/anim_stand.mv1", "idle");
		ModelManager::LoadAsAnimation("data/player/anim_walk.mv1", "walk");
		ModelManager::LoadAsAnimation("data/player/anim_run.mv1", "run");
		ModelManager::LoadAsAnimation("data/player/anim_jump.mv1", "jump");
		ModelManager::LoadAsAnimation("data/player/anim_salute.mv1", "salute");
		ModelManager::LoadAsAnimation("data/player/anim_aim.mv1", "aim");
		ModelManager::LoadAsAnimation("data/player/anim_reload.mv1", "reload");
		AudioManager::Load("data/Sound/reload.mp3", "reload_se", false);
		loading_status = (ModelManager::GetLoadingCount() == 0 && AudioManager::GetLoadingCount() == 0) ? LOADING_STATUS::LOADED : LOADING_STATUS::LOADING;

	}

	int SampleAnimationScene::Init()
	{
		if (!CheckForLoading())
			return 0;
		player = SceneManager::Object::Create<SampleAnimationObject>();
		player->transform->position = Vector3(5, 0, 0);
		player2 = SceneManager::Object::Create<SampleAnimationObject>();
		player2->transform->position = Vector3(-5, 0, 0);
		player3 = SceneManager::Object::Create<SampleAnimationObject>();

		camera = SceneManager::Object::Create<CameraObject>();
		//SceneManager::Object::Create<DebugCameraObject>();
		camera->transform->position = Vector3(0, 5, -10);

		return 0;
	}

	void SampleAnimationScene::Update()
	{
		if (!CheckForLoading())
			return;
		auto camera_sp = camera.lock();
		if (Input::GetKey(KeyCode::Left))
			player->transform->local_position.x -= Time::DeltaTime() * 5;
		if (Input::GetKey(KeyCode::Right))
			player->transform->local_position.x += Time::DeltaTime() * 5;
		if (Input::GetKey(KeyCode::Up))
			player2->transform->local_position.x += Time::DeltaTime() * 5;
		if (Input::GetKey(KeyCode::Down))
			player2->transform->local_position.x -= Time::DeltaTime() * 5;

#if 1

		if (Input::GetKey(KeyCode::A))
			camera->transform->MovePosition(camera->transform->AxisX() * -5);
		if (Input::GetKey(KeyCode::D))
			camera->transform->MovePosition(camera->transform->AxisX() * 5);
		if (Input::GetKey(KeyCode::W))
			camera->transform->MovePosition(camera->transform->AxisZ() * 5);
		if (Input::GetKey(KeyCode::S))
			camera->transform->MovePosition(camera->transform->AxisZ() * -5);
		if (Input::GetKey(KeyCode::Q))
			camera->transform->MovePosition(camera->transform->AxisY() * -5);
		if (Input::GetKey(KeyCode::E))
			camera->transform->MovePosition(camera->transform->AxisY() * 5);
		if (Input::GetKey(KeyCode::Left))
			camera->transform->AddRotation(Vector3(0, -90 * Time::DeltaTime(), 0));
		if (Input::GetKey(KeyCode::Right))
			camera->transform->AddRotation(Vector3(0, 90 * Time::DeltaTime(), 0));
#endif

		if (Input::GetKeyDown(KeyCode::Space)) {
			if (player->transform->GetChild(0).lock())
				player2->transform->SetChild(player3->transform);
			else
				player->transform->SetChild(player3->transform);
		}

	}

	void SampleAnimationScene::Draw()
	{
		if (!CheckForLoading())
			DrawString(50, 50, "Now Loading...", Color::RED);
	}

	void SampleAnimationScene::LateDraw()
	{
		DrawFormatString(0, 0, Color::RED, "FPS:%f", Time::GetFPS());
	}

	void SampleAnimationScene::Exit()
	{
	}

}