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
		player->transform->position = Vector3(10, 0, 0);
		player2 = SceneManager::Object::Create<SampleAnimationObject>();
		player2->transform->position = Vector3(-10, 0, 0);
		player3 = SceneManager::Object::Create<SampleAnimationObject>();

		camera = SceneManager::Object::Create<CameraObject>();
		//SceneManager::Object::Create<DebugCameraObject>();
		camera->transform->position = Vector3(0, 10, -20);
		camera->transform->SetAxisZ(-camera->transform->position.getNormalized());

		return 0;
	}

	void SampleAnimationScene::Update()
	{
		if (!CheckForLoading())
			return;
		auto camera_sp = camera.lock();


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
		Vector2 rot_ratio = { 0,0 };
		if (Input::GetKey(KeyCode::Left))
			rot_ratio.y -= Time::DeltaTime()*60;
		if (Input::GetKey(KeyCode::Right))
			rot_ratio.y += Time::DeltaTime()*60;
		if (Input::GetKey(KeyCode::Up))
			rot_ratio.x -= Time::DeltaTime()*60;
		if (Input::GetKey(KeyCode::Down))
			rot_ratio.x += Time::DeltaTime()*60;
		camera->transform->AddRotation({ rot_ratio.x,rot_ratio.y,0 });
		camera->transform->SetAxisZ(camera->transform->AxisZ());
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
		DrawString(0, 21, ShiftJISToUTF8("テンキーの1~7でそれぞれのアニメーションを再生します").c_str(), Color::YELLOW);
		DrawString(0, 42, ShiftJISToUTF8("4,5,7のアニメーションはコールバックで音が鳴ったり、次のアニメーションに勝手に推移します").c_str(), Color::YELLOW);
		DrawString(0, 66, ShiftJISToUTF8("エンターキーでアニメーションの再生、逆再生を切り替えられます").c_str(), Color::YELLOW);

	}

	void SampleAnimationScene::Exit()
	{
	}

}