#include "precompile.h"
#include "SceneAme.h"
#include "System/Components/ModelRenderer.h"
#include "System/Components/RigidBody.h"
#include "System/Components/MeshCollider.h"
#include "System/Objects/ButtonObject.h"
#include "System/Objects/TextObject.h"
#include "System/Shader.h"
#include "Game/Objects/AmeGame/AmePlayer.h"
#include "Game/Objects/AmeGame/AmeManager.h"
#include "System/IniFileManager.h"


#include "System/Components/CapsuleCollider.h"
#include "System/Components/SphereCollider.h"
#include "Game/Objects/AmeGame/IventManager.h"

#include "System/Objects/ShadowMapObject.h"

/*
 長い岩1
9696,-181,-2650
0,4.712,0
0.68,0.68,0.58

長い岩2
10443,-339.825,-3398.252
0,0.224,0
0.84,0.84,0.84

デカい岩1
9311,0,-3670
0,0,0
0.48,0.48,0.48

デカい岩2
10013,0,-3874
0,3.276,0
0.48,0.48,0.48

小っちゃい岩
9628,249,-3783
0,0.494,0
10.44,10.44,10.44

神社
9714,362,-3171
0,1.571,0
102,102,102
*/



inline std::string u8str(const char8_t* str) {
	return std::string(reinterpret_cast<const char*>(str));
}



namespace AmeGame
{
	GameObjectP stage_object = nullptr;		//配置テスト用

	struct CameraInfo
	{
		mat4x4 mat_view_; //!< ビュー行列
		mat4x4 mat_proj_; //!< 投影行列
		Vector3 eye_position_; //!< カメラの位置
		float a;
	};
	struct LightInfo
	{
		mat4x4  mat_view_proj_; //!< ビュー投影行列
		Vector4 light_color_; //!< 光源の色
		Vector3 light_direction_; //!< 光源の方向
		float a;
	};

	//---------------------------------------------------------------------------
//!  [左手座標系] ビュー行列
//---------------------------------------------------------------------------
	mat4x4 lookAtLH(const Vector3& eye, const Vector3& lookAt, const Vector3& worldUp)
	{
		Vector3 axis_z = (lookAt - eye).getNormalized();
		Vector3 axis_x = worldUp.cross(axis_z).getNormalized();
		Vector3 axis_y = axis_z.cross(axis_x);

		f32 tx = -axis_x.dot(eye);
		f32 ty = -axis_y.dot(eye);
		f32 tz = -axis_z.dot(eye);

		Vector4 m[4]{
			{axis_x.x, axis_y.x, axis_z.x, 0.0f},
			{axis_x.y, axis_y.y, axis_z.y, 0.0f},
			{axis_x.z, axis_y.z, axis_z.z, 0.0f},
			{      tx,       ty,       tz, 1.0f},
		};
		return mat4x4(m[0], m[1], m[2], m[3]);
	}

	//---------------------------------------------------------------------------
	//! [左手座標系] 平行投影行列
	//---------------------------------------------------------------------------
	mat4x4 orthographicOffCenterLH(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z)
	{
		float rcp_width = 1.0f / (right - left);
		float rcp_height = 1.0f / (top - bottom);
		float range = 1.0f / (far_z - near_z);

		Vector4 m[4]{
			{           rcp_width * 2.0f,                         0.0f,            0.0f, 0.0f},
			{                       0.0f,            rcp_height * 2.0f,            0.0f, 0.0f},
			{                       0.0f,                         0.0f,           range, 0.0f},
			{-(left + right) * rcp_width, -(top + bottom) * rcp_height, -range * near_z, 1.0f}
		};
		return mat4x4(m[0], m[1], m[2], m[3]);
	}

	//---------------------------------------------------------------------------
	//! [左手座標系] 投影行列
	//---------------------------------------------------------------------------
	mat4x4 perspectiveFovLH(f32 fovy, f32 aspect_ratio, f32 near_z, f32 far_z)
	{
		f32 s = std::sinf(fovy * 0.5f);
		f32 c = std::cosf(fovy * 0.5f);

		f32 height = c / s;
		f32 width = height / aspect_ratio;
		f32 range = far_z / (far_z - near_z);

		Vector4 m[4]{
			{width,   0.0f,            0.0f, 0.0f},
			{ 0.0f, height,            0.0f, 0.0f},
			{ 0.0f,   0.0f,           range, 1.0f},
			{ 0.0f,   0.0f, -range * near_z, 0.0f}
		};
		return mat4x4(m[0], m[1], m[2], m[3]);
	}





	class FadeInOutObject :public GameObject {
	public:
		USING_SUPER(FadeInOutObject);
		Color panel_color = Color::BLACK;
		bool in = true;
		float fade_timer = 1.0f;
		float fade_timer_max = 1.0f;
		int Init() override {
			panel_color.a = 1.0f;
			SetPriority(49);
			return 1;
		}
		void FadeInStart(float timer = 1.0f) {
			fade_timer = timer;
			fade_timer_max = timer;
			panel_color.a = 1.0f;
			in = true;
		}
		void FadeOutStart(float timer = 1.0f) {
			fade_timer = timer;
			fade_timer_max = timer;
			in = false;
			panel_color.a = 0.0f;
		}
		void Update() {
			if (fade_timer > 0) {
				fade_timer -= Time::DeltaTime();
				panel_color.a = in ? (fade_timer / fade_timer_max) : (fade_timer_max - fade_timer) / fade_timer_max;
			}
		}
		void LateDraw() {
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, panel_color.a * 255);
			DrawFillBox(0, 0, SCREEN_W, SCREEN_H, panel_color);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}

	};


	class TreeObject :public GameObject {
	public:
		USING_SUPER(TreeObject);
		inline int Init() override {

			auto root = SceneManager::Object::Get<GameObject>("root");

			auto model = AddComponent<ModelRenderer>();
			model->SetModel("tree");
			transform->scale = { 0.035f,0.035f,0.035f };
			auto rb = AddComponent<RigidBody>();
			rb->ChangeToStatic();
			auto col = AddComponent<CapsuleCollider>();
			col->radius = 1.5f;
			col->height = 10.f;
			col->rotation = Quaternion(DEG2RAD(90), { 0,0,1 });
			col->position = { 5, 0, 0 };
			//root->transform->SetChild(transform);
			return true;
		}
	};
	class OnmyouDamaObject :public GameObject {
	public:
		USING_SUPER(OnmyouDamaObject);
		SafeSharedPtr<Texture> onmyoudama;
		std::string popup_text = "";
		std::function<void()> interact_func = nullptr;
		std::function<bool()> another_trigger = nullptr;
		bool is_active = false;
		inline int Init() override {
			onmyoudama = TextureManager::CloneByName("onmyoudama");
			auto rb = AddComponent<RigidBody>();
			rb->is_kinematic = true;
			auto trigger = AddComponent<SphereCollider>();
			trigger->is_trigger = true;
			trigger->radius = 1.0f;
			return 1;
		}
		inline void Update() override {
			bool can_interact = is_active;
			if (another_trigger)
				can_interact = can_interact && another_trigger();
			if (can_interact && Input::GetKeyDown(KeyCode::Return)) {
				if (interact_func)
					interact_func();
			}

		}
		inline void Draw() override {
			Vector3 pos = transform->position;
			float scale = transform->scale.magnitude();
			SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
			DrawRotaGraph3D(pos.x, pos.y, pos.z, 0.003f * scale, Time::GetTimeFromStart(), *onmyoudama, true);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}
		inline void LateDraw() {
			bool is_draw = is_active;
			if (another_trigger)
				is_draw = is_draw && another_trigger();
			if (is_draw) {
				int font_size = GetFontSize();
				SetFontSize(28);
				SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
				DrawFillBox(SCREEN_W * 0.6f, SCREEN_H * 0.6f, SCREEN_W * 0.6f + 10 * popup_text.length(), SCREEN_H * 0.6f + 28, Color::WHITE);
				SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
				DrawString(SCREEN_W * 0.6f, SCREEN_H * 0.6f, popup_text.c_str(), Color::BLACK);
				SetFontSize(font_size);
			}
		}
		inline void OnTriggerEnter(const HitInfo& hit_info) override {
			if (hit_info.hit_collision->owner->name == "Player")
				is_active = true;

		}
		inline void OnTriggerExit(const HitInfo& hit_info) override {
			if (hit_info.hit_collision->owner->name == "Player")
				is_active = false;
		}
		inline void Exit() override {
			onmyoudama.reset();
		}
	};



	std::string font = "青柳疎石フォント2";
	SafeSharedPtr<AudioClip> title_audio = nullptr;
	std::string txt = "";//u8str(u8"  八千八声\n        啼いて<c900>血<c>を吐く\n　　　　　　ホトトギス");

	void SceneAme::Load()
	{
		//なんかDegRad変換がおかしい気がする->テスト
		float rad = PI; //
		float deg = RAD2DEG(rad); //
		rad = DEG2RAD(deg); //
		ModelManager::LoadAsModel("data/Stage/Sky_Dome.mv1", "sky");
		ModelManager::LoadAsModel("data/Stage/Gensokyo/Gensokyo1.mv1", "field");
		ModelManager::LoadAsModel("data/Stage/Gensokyo/hitozato.mv1", "field2");
		ModelManager::LoadAsModel("data/Stage/HakureiShrine/HakureiShrine1.mv1", "shrine");
		ModelManager::LoadAsModel("data/Stage/SwordBout/nagai_iwa.mv1", "long_rock");
		ModelManager::LoadAsModel("data/Stage/SwordBout/dekai_iwa.mv1", "huge_rock");
		ModelManager::LoadAsModel("data/Stage/SwordBout/chicchai_iwa.mv1", "small_rock");
		ModelManager::LoadAsModel("data/Stage/SwordBout/tree.mv1", "tree");
		ModelManager::LoadAsModel("data/Stage/Chabudai/chabudai.mv1", "chabudai");
		AudioManager::Load("data/Sound/Hototogisu.mp3", "title");
		AudioManager::Load("data/Sound/rain.mp3", "rain");
		TextureManager::Load("data/Stage/onmyoudama.png", "onmyoudama");

		loading_status = (ModelManager::GetLoadingCount() == 0 && AudioManager::GetLoadingCount() == 0) ? LOADING_STATUS::LOADED : LOADING_STATUS::LOADING;
	}

	void InitStageUnits(TransformP root, std::string mod_name, Vector3 pos, Quaternion rot, Vector3 scale) {

		auto stage = SceneManager::Object::Create<Object>("Stage");
		stage->transform->position = pos;
		stage->transform->rotation = rot;
		stage->transform->scale = scale;
		auto model = stage->AddComponent<ModelRenderer>();
		model->SetModel(mod_name);
		stage->AddComponent<RigidBody>();
		auto col = stage->AddComponent<MeshCollider>();
		col->AttachToModel();
		root->SetChild(stage->transform); //ステージをシーンのルートに配置

	}
	int SceneAme::Init()
	{

		if (!CheckForLoading())
			return 0;
		SceneManager::Object::Create<ShadowMapObject>()->SetShadowMapSize(1024);
		title_audio = AudioManager::CloneByName("title");

		//地形ルートパーツ
		auto root = SceneManager::Object::Create<GameObject>("root");

		{//地形の子供となるパーツの初期化
			InitStageUnits(root->transform, "field", { 0,0,0 }, { 0,0,0,1 }, { 1,1,1 });
			InitStageUnits(root->transform, "field2", { 1546.202f,-11.327f,-4746.206f }, Quaternion(6.264f, { 0,1,0 }), { 13.18f,13.18f,13.18f });
			InitStageUnits(root->transform, "shrine", { 9800,362,-3171 }, Quaternion(1.571f, { 0,1,0 }), { 102,102,102 });
			InitStageUnits(root->transform, "long_rock", { 10543,-339.8f,-3398.2f }, Quaternion(0.224f, { 0,1,0 }), { 0.84f,0.84f,0.84f });
			InitStageUnits(root->transform, "long_rock", { 9796,-181,-2650 }, Quaternion(4.712f, { 0,1,0 }), { 0.68f,0.68f,0.68f });
			InitStageUnits(root->transform, "huge_rock", { 9401,0,-3670 }, { 0,0,0,1 }, { 0.48f,0.48f,0.48f });
			InitStageUnits(root->transform, "huge_rock", { 10103,0,-3874 }, Quaternion(3.276f, { 0,1,0 }), { 0.48f,0.48f,0.48f });
			InitStageUnits(root->transform, "small_rock", { 9728,249,-3783 }, Quaternion(0.494f, { 0,1,0 }), { 10.44f,10.44f,10.44f });
			InitStageUnits(root->transform, "chabudai", { 10091.94f,395.87f,-3378.63f }, Quaternion(DEG2RAD(90), { 1,0,0 }), { 18.14f,18.14f,18.14f });
		}
		root->transform->scale = { 0.1f,0.1f,0.1f };
		camera = SceneManager::Object::Create<AmePlayer>("Player");
		if constexpr (false)
		{
			stage_object = SceneManager::Object::Create<GameObject>();
			auto model = stage_object->AddComponent<ModelRenderer>();
			model->SetModel("tree");
			stage_object->transform->position = { 900,21,-350 };
		}
		SceneManager::Object::Create<AmeGame::AmeManager>();
		for (int i = 0; i < 5; i++) {
			Vector3 pos = FileSystem::IniFileManager::GetVector3("GameObject" + std::to_string(i), "position", { 0,0,0 }, "data/Object.ini");
			SceneManager::Object::Create<AmeGame::TreeObject>()->transform->position = pos;

		}
		auto fadeobj = SceneManager::Object::Create<FadeInOutObject>();
		fadeobj->FadeInStart(3);

		{
			txt = FileSystem::IniFileManager::GetString("TEXT", "text0", "", "data/Object.ini");
			txtnavi = SceneManager::Object::Create<TextObject>();
			txtnavi->CanvasAnchorType() = UIObject::ANCHOR_TYPE::CENTER;
			txtnavi->AnchorType() = UIObject::ANCHOR_TYPE::CENTER;
			auto text = txtnavi->GetComponent<Text>();
			text->SetFont(font);
			text->TextColor() = Color::WHITE;
			text->SetText(txt);
			text->SetAlignment(Text::ALIGNMENT::AUTO);
			text->FontSize() = 100;
			text->text_speed = 7;
			text->ResetDrawChar();
		}


		iv_manager = SceneManager::Object::Create<AmeGame::IventManager>();
		iv_manager->SetPriority(10);
		std::vector<IventManager::IventStartAndEnd> info_;

		{
			for (size_t section_num = 0;; section_num++) {
				IventManager::IventStartAndEnd info;
				std::string section = "TEXT" + std::to_string(section_num);
				if (FileSystem::IniFileManager::GetString(section, "text0", "", "data/Object.ini").empty())
					break;
				for (size_t key_num = 0;; key_num++) {
					std::string key = "text" + std::to_string(key_num);
					std::string text_data = FileSystem::IniFileManager::GetString(section, key, "", "data/Object.ini");
					if (text_data.empty())
						break;
					info.ivent_texts.push_back(text_data);
				}
				info_.push_back(info);
			}
			iv_manager->RegisterIvents(std::move(info_));

		}

		{
			auto onmyou = SceneManager::Object::Create<AmeGame::OnmyouDamaObject>();
			onmyou->transform->position = { 1008.f,40.f,-337.8f };
			onmyou->transform->scale = { 0.1f,0.1f,0.1f };
			onmyou->popup_text = u8str(u8"見る");
			auto&& lambda0 = [manager = SafeWeakPtr(iv_manager), &active = onmyou->is_active, &interact = interact_num]() {
				active = false;
				manager->GetIvent(1)->on_ivent_finish = std::move([&]() {interact = 1; });
				manager->IventStart(1); };
			onmyou->interact_func = std::move(lambda0);
			auto&& lambda1 = [manager = SafeWeakPtr(iv_manager)]()->bool {
				return !manager->is_on_ivent;
				};
			onmyou->another_trigger = std::move(lambda1);
			interact_points.push_back(onmyou->transform->position);
		}
		{
			auto onmyou = SceneManager::Object::Create<AmeGame::OnmyouDamaObject>();
			onmyou->transform->position = { 886, 20, -315 };
			onmyou->popup_text = u8str(u8"->村へ出る");
			auto&& lambda0 = [manager = SafeWeakPtr(iv_manager), &player = camera, &active = onmyou->is_active, &interact = interact_num]() {
				manager->IventStart(2);
				active = false;
				manager->GetIvent(2)->on_ivent_finish = std::move([&]() {interact = 2; });
				if (player)
					player->transform->SetPosition({ 158,1,-425 });
				};
			onmyou->interact_func = std::move(lambda0);
			interact_points.push_back(onmyou->transform->position);
		}
		{
			auto onmyou = SceneManager::Object::Create<AmeGame::OnmyouDamaObject>();
			onmyou->transform->position = { 33,1,-554 };
			onmyou->popup_text = u8str(u8"->ロープウェイへ");
			auto&& lambda0 = [manager = SafeWeakPtr(iv_manager), &player = camera, &active = onmyou->is_active, &interact = interact_num]() {
				manager->IventStart(3);
				active = false;
				manager->GetIvent(3)->on_ivent_finish = std::move([&]() {interact = 3; });
				if (player)
					player->transform->SetPosition({ -317,7,-550 });
				};
			onmyou->interact_func = std::move(lambda0);
			interact_points.push_back(onmyou->transform->position);

		}
		{
			auto onmyou = SceneManager::Object::Create<AmeGame::OnmyouDamaObject>();
			onmyou->transform->position = { -323,7,-556 };
			onmyou->popup_text = u8str(u8"->乗る");
			auto&& lambda0 = [manager = SafeWeakPtr(iv_manager), &player = camera, &active = onmyou->is_active, &interact = interact_num]() {
				manager->IventStart(4);
				active = false;
				manager->GetIvent(4)->on_ivent_finish = std::move([&]() {interact = 4; player->transform->position = { -320,385,465 }; });
				};
			onmyou->interact_func = std::move(lambda0);
			interact_points.push_back(onmyou->transform->position);

		}
		{
			auto onmyou = SceneManager::Object::Create<AmeGame::OnmyouDamaObject>();
			onmyou->transform->position = { -319,383,470 };
			onmyou->popup_text = u8str(u8"->山頂へ");
			auto&& lambda0 = [&, manager = SafeWeakPtr(iv_manager), &player = camera, &active = onmyou->is_active, &interact = interact_num]() {
				manager->IventStart(5);
				active = false;
				manager->GetIvent(5)->on_ivent_finish = std::move([&]() {interact = 5; fadeobj->FadeOutStart(3); manager->IventStart(6); });
				if (player) {
					player->transform->SetPosition({ 33, 769, 613 });
					player->transform->SetRotation({ 0, -90, 0 });
				}
				};
			onmyou->interact_func = std::move(lambda0);
			interact_points.push_back(onmyou->transform->position);

		}
		//{ 40, 769, 612 };





		if (title_audio)
			title_audio->PlayOneShot();


		txtnavi->transform->scale = { 1300,301,1 };
		txtnavi->transform->position = { 0,0,0 };


		return 0;
	}

	void SceneAme::Update()
	{
		if (!CheckForLoading())
			return;
#if 0
		if (Input::GetKey(KeyCode::Up))
			stage_object->transform->position.z += Time::DeltaTime() * 3;
		if (Input::GetKey(KeyCode::Down))
			stage_object->transform->position.z += Time::DeltaTime() * -3;
		if (Input::GetKey(KeyCode::Left))
			stage_object->transform->position.x += Time::DeltaTime() * -3;
		if (Input::GetKey(KeyCode::Right))
			stage_object->transform->position.x += Time::DeltaTime() * 3;
		if (Input::GetKey(KeyCode::O))
			stage_object->transform->position.y += Time::DeltaTime() * 3;
		if (Input::GetKey(KeyCode::P))
			stage_object->transform->position.y += Time::DeltaTime() * -3;

		if (Input::GetKey(KeyCode::U)) {
			stage_object->transform->scale += {Time::DeltaTime(), Time::DeltaTime(), Time::DeltaTime()};
		}
		if (Input::GetKey(KeyCode::I)) {
			stage_object->transform->scale -= {Time::DeltaTime(), Time::DeltaTime(), Time::DeltaTime()};
		}

		if (Input::GetKeyDown(KeyCode::Return)) {
			FileSystem::IniFileManager::SetVector3(stage_object->name + std::to_string(text_num), "position", stage_object->transform->position, "data/Object.ini");
			FileSystem::IniFileManager::SetVector3(stage_object->name + std::to_string(text_num), "scale", stage_object->transform->scale, "data/Object.ini");
		}

		if (Input::GetKeyDown(KeyCode::Return)) {
			std::string key = "text";
			key += std::to_string(text_num);
			txt = FileSystem::IniFileManager::GetString("TEXT", key, "", "data/Object.ini");
			text_num++;
			auto text = txtnavi->GetComponent<Text>();
			text->FontSize() = 28;
			text->text_speed = 30;
			text->SetFont("");
			text->SetText(txt);
			text->ResetDrawChar();
			txtnavi->transform->position = { 0,0,0 };
			txtnavi->AnchorType() = UIObject::ANCHOR_TYPE::LEFT_BOTTOM;
			txtnavi->CanvasAnchorType() = UIObject::ANCHOR_TYPE::LEFT_BOTTOM;


		}
#endif
		if (Input::GetKeyDown(KeyCode::Return) && interact_num == -1) {

			txtnavi->transform->position = { 0,0,0 };
			txtnavi->AnchorType() = UIObject::ANCHOR_TYPE::LEFT_BOTTOM;
			txtnavi->CanvasAnchorType() = UIObject::ANCHOR_TYPE::LEFT_BOTTOM;
			SceneManager::Object::Get<IventManager>()->IventStart(0);
			interact_num++;
		}
		else if (Input::GetKeyDown(KeyCode::Return)) {
			if (interact_num < interact_points.size() && !iv_manager->is_on_ivent) {
				arrow_timer = 1.0f;
			}
		}
		if (arrow_timer > 0.0f)
			arrow_timer -= Time::DeltaTime();
	}

	void SceneAme::Draw()
	{
		if (!CheckForLoading())
			return;



		if (arrow_timer > 0.0f && interact_num < interact_points.size()) {
			Vector3 arrow_start = camera->transform->position + camera->transform->AxisZ() * 0.1f - camera->transform->AxisY() * 0.7f;
			Vector3 arrow_vec = ProjectOnPlane(interact_points[interact_num] - arrow_start, { 0,1,0 });
			arrow_vec.normalizeSafe();
			arrow_vec *= 0.2f;
			SetDrawBlendMode(DX_BLENDMODE_ADD, 255 * arrow_timer);
			SetUseLighting(false);
			SetLightEnable(false);
			DrawLine3D(cast(arrow_start), cast(arrow_start + arrow_vec), Color::BLUE);
			DrawCone3D(cast(arrow_start + arrow_vec), cast(arrow_start + arrow_vec * 0.7f), 0.01f, 32, Color::BLUE, Color::BLUE, true);
			SetUseLighting(true);
			SetLightEnable(true);
			SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		}


	}

	void SceneAme::LateDraw()
	{
		if (!CheckForLoading()) {
			std::string str("Loading");
			for (int i = 0; i < (int)(Time::GetTimeFromStart() * 10) % 5; i++)
				str += ".";
			DrawFormatString(SCREEN_W * 0.5f, SCREEN_H * 0.5f, Color::RED, "%s", str.c_str());
			return;
		}
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 192);
		//	DrawFillBox(0, 0, SCREEN_W, SCREEN_H, Color::BLACK);
		DrawString(10, 10, "SceneAme", Color::WHITE);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		{
			DrawFormatString(10, SCREEN_H - 100, Color::WHITE, u8str(u8"WASDで移動").c_str());
			DrawFormatString(10, SCREEN_H - 80, Color::WHITE, u8str(u8"マウス移動でカメラ回転").c_str());
			DrawFormatString(10, SCREEN_H - 60, Color::WHITE, u8str(u8"スペースキーでジャンプ").c_str());
			DrawFormatString(10, SCREEN_H - 40, Color::WHITE, u8str(u8"困ったらEnterキーを押しましょう").c_str());

		}

	}

	void SceneAme::UnLoad()
	{
		title_audio.reset();
		stage_object.reset();
	}

	bool SceneAme::CheckForLoading()
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

}