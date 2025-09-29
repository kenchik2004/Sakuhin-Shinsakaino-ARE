#include "precompile.h"
#include "SceneTitle.h"
#include "Game/Scenes/RLyeh/DiceScene.h"
#include "System/Components/ModelRenderer.h"
#include <System/IniFileManager.h>
#include <System/Components/Camera.h>
namespace RLyeh {

	void SceneTitle::Load()
	{
		AudioManager::Load("data/Sound/title.mp3", "title");
		TextureManager::Load("data/title.jpg", "title_img");
		title_img = TextureManager::CloneByName("title_img");
		loading_status = ModelManager::GetLoadingCount() + AudioManager::GetLoadingCount() == 0 ? LOADING_STATUS::LOADED : LOADING_STATUS::LOADING;
		dice_scene = SceneManager::LoadAsAnother<DiceScene>();
	}

	int SceneTitle::Init()
	{
		loading_status = ModelManager::GetLoadingCount() + AudioManager::GetLoadingCount() == 0 ? LOADING_STATUS::LOADED : LOADING_STATUS::LOADING;
		if (loading_status == LOADING_STATUS::LOADING && !cam) {
			cam = SceneManager::Object::Create<GameObject>()->AddComponent<Camera>();
			return 0;
		}
		bgm = AudioManager::CloneByName("title");
		bgm->PlayOneShot();
		return 0;
	}

	static float t = 0;
	void SceneTitle::Update()
	{
		if (loading_status == LOADING_STATUS::LOADING) {
			loading_status = ModelManager::GetLoadingCount() + AudioManager::GetLoadingCount() == 0 ? LOADING_STATUS::LOADED : LOADING_STATUS::LOADING;
			if (loading_status == LOADING_STATUS::LOADED)
				Init();
			else
				return;
		}

		t += Time::DeltaTime();
		//とりあえず技術検証用に裏シーンとのやり取り
		if (Input::GetKeyDown(KeyCode::Space)) {
			dice_scene->Clear();
			dice_scene->Roll(20, 100);
			//dice_scene->Roll(2, 10);
			//dice_scene->Roll(2, 4);
			//dice_scene->Roll(1, 100);
			//dice_scene->Roll(1, 12);
			//dice_scene->Roll(1, 8);
		}
		if (Input::GetKeyDown(KeyCode::Alpha1)) {
			dice_scene->Skip();
			results.clear();
			results = dice_scene->FetchResults();
			//std::sort(results.begin(), results.end(), [](int a, int b) {return a < b; });
		}

	}
	void SceneTitle::LateDraw()
	{
		//タイトル画面の描画
		if (title_img)
			DrawExtendGraph(0, 0, SCREEN_W, SCREEN_H, title_img->GetHandle(), false);

		//なんかそれっぽく暗めにする
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 192);
		DrawBox(0, 0, SCREEN_W, SCREEN_H, Color::BLACK, true);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

		//ロード中
		if (loading_status == LOADING_STATUS::LOADING) {
			std::string str("Loading");
			for (int i = 0; i < (int)(Time::GetTimeFromStart() * 2) % 5; i++)
				str += ".";
			DrawFormatString(SCREEN_W * 0.5f, SCREEN_H * 0.5f, Color::RED, "%s", str.c_str());
		}
		else {
			//裏シーンの描画領域をこっちに持ってくる
			DrawExtendGraph(0, 0, SCREEN_W, SCREEN_H, SafeStaticCast<Camera>(dice_scene->GetCurrentCamera().lock())->my_screen->GetHandle(), true);
			//デカデカとタイトル表示
			SetFontSize(150);
			int x, y;
			GetDrawStringSize(&x, &y, nullptr, "  THE\nR'LYEH", 13);
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, fabsf(sinf(static_cast<float>(Time::GetTimeFromStart())) * 255));
			DrawString(SCREEN_W * 0.5f - x * 0.5f, SCREEN_H * 0.5f - y * 0.5f, "  THE\nR'LYEH", Color(1, 0, 0, 1));
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
			SetFontSize(DEFAULT_FONT_SIZE);
			//とりあえず1でスキップ&結果集計
			int i = 0;
			int sum = 0;
			for (auto& ite : results) {
				DrawFormatString(0, i * 16, Color::CYAN, "%d,", ite);
				i++;
				sum += ite;
			}
			DrawFormatString(0, i * 16, Color::CYAN, "%d", sum);
			DrawFormatString(100, 16, Color::CYAN, "%f", Time::GetFPS());
			DrawFormatString(100, 32, Color::CYAN, "%f", Time::DeltaTime());

		}
		DrawFormatString(100, 48, Color::CYAN, "%f", t);
		DrawFillBox(100, 64, 100 + t*50, 80, Color::RED);
		for(int i=0;i<20;i++)
		DrawLine(100+50*i, 64, 100+50*i, 80, Color::RED);
	}

	void SceneTitle::UnLoad()
	{
		bgm = nullptr;
		title_img.reset();
	}

}