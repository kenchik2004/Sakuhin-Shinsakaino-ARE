#include "Main.h"
#include "System/SceneManager.h"
#include <fstream>
#include "System/IniFileManager.h"
#include "System/ImGui_.h"
#include <System/Components/Camera.h>
//#define DEBUG_WINDOW
//#define USE_DEBUG_DRAW
//#define FULL_SCREEN

//#define SECONDARY

std::string window_classname[1] =
{
	"デバッグウィンドウ1",
};
int CreateDebugWindow(HINSTANCE& hInstance, HWND& window, int window_x, int window_y, WNDCLASS& window_parameter, int nCmdShow);
//====================================//

// メッセージ処理用関数
constexpr LRESULT CALLBACK WndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_MOVING:
	case WM_SIZE:
		//ウィンドウ移動中は時飛ばしを行う(Physicsやアップデート処理の暴走を防ぐため)
		Time::ResetTime();
		break;
	default:
		return(DefWindowProc(window, msg, wParam, lParam));
	}
	return (0L);
}
constexpr LRESULT CALLBACK DxWndProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_MOVING:
	case WM_SIZE:
		//ウィンドウ移動中は時飛ばしを行う(Physicsやアップデート処理の暴走を防ぐため)
		Time::ResetTime();
		break;
	}
	return (0L);
}
//=====================================//
//---------------------------------------------------------------------------------
//	WinMain
//---------------------------------------------------------------------------------
int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
	
	//もうUTF-8しか使わん!SHIFT_JISはクソ!!
	SetUseCharCodeFormat(DX_CHARCODEFORMAT_UTF8);

	//==================================//
#ifdef DEBUG_WINDOW
	MSG msg;
	HWND window[1];
	WNDCLASS param;
#endif
	SRand((unsigned int)time(nullptr));
	SetOutApplicationLogValidFlag(FALSE);

	bool not_full_screen = FileSystem::IniFileManager::GetBool("StartConfig", "full_screen", false, "data/test.ini");
	ChangeWindowMode(!not_full_screen);

#ifdef FULL_SCREEN
	//ChangeWindowMode(false);
#endif
	SetGraphMode(SCREEN_W*0.5f, SCREEN_H*0.5f, 32, 240);
	std::string window_text = FileSystem::IniFileManager::GetString("StartConfig", "window_name", "メインウィンドウ", "data/test.ini");
	SetMainWindowText(window_text.c_str());
	SetBackgroundColor(100, 100, 100);
	//SetWindowStyleMode(4);
	//SetWindowSizeChangeEnableFlag(true, true);
	SetHookWinProc(DxWndProc);
	SetDoubleStartValidFlag(true);
	SetAlwaysRunFlag(TRUE);
	SetWaitVSyncFlag(false);
	Set3DSoundOneMetre(1.0f);
	SetEnableXAudioFlag(true);
	SetUseDirect3DVersion(DX_DIRECT3D_11);

	if (DxLib_Init() == -1)	return -1;

	SetWindowSizeChangeEnableFlag(true, false);


	bool show_mouse = FileSystem::IniFileManager::GetBool("StartConfig", "show_mouse", true, "data/test.ini");

	SetMouseDispFlag(show_mouse);

#ifdef DEBUG_WINDOW


	RECT rect;
	GetWindowCRect(&rect);
	if (CreateDebugWindow(hInstance, window[0], rect.right - rect.left, rect.bottom - rect.top, param, nCmdShow) == -1) return -1;
#endif // DEBUG_WINDOW


	timeBeginPeriod(1);
	SetDrawScreen(DX_SCREEN_BACK);
	SetTransColor(255, 0, 255);
	srand(GetNowCount() % RAND_MAX);
	//SetWindowPosition(0, 0);

	Time::Init();
	Input::Init();
	PhysicsManager::Init();
	SceneManager::Init();
#if 0
	ImGuiInit(false);
#endif
	//描画のFPSを設定
	int d_fps = FileSystem::IniFileManager::GetInt("StartConfig", "draw_fps", 60, "data/test.ini");
	Time::SetDrawFPSMAX(d_fps);

	//内部処理のFPSを設定
	int fps = FileSystem::IniFileManager::GetInt("StartConfig", "update_fps", 60, "data/test.ini");
	Time::SetFPSMAX(fps);

	int fix_fps = FileSystem::IniFileManager::GetInt("StartConfig", "fixed_fps", 50, "data/test.ini");
	Time::SetFixedFPSMAX(fix_fps);

	int time_scale = FileSystem::IniFileManager::GetInt("StartConfig", "time_scale", 1, "data/test.ini");
	Time::SetTimeScale(time_scale);


	SetCameraNearFar(0.1f, 3000.0f);
	SetupCamera_Perspective(TO_RADIAN(45.0f));

	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);
	ChangeLightTypeDir(VGet(0.8f, -1.2f, 1.0f));

	SetCameraPositionAndTarget_UpVecY(float3(0, 0, 0), float3(0, 0, 1));

	SetUseSetDrawScreenSettingReset(false);

	//===============================================//
#ifndef SECONDARY 

	auto start_scene_name = FileSystem::IniFileManager::GetString("StartConfig", "start_scene", "SceneSample", "data/test.ini");
#else
	auto start_scene_name = FileSystem::IniFileManager::GetString("StartConfig", "secondary_start_scene", "SceneSample", "data/test.ini");

#endif
	auto start_scene = CreateInstanceFromName<Scene>(start_scene_name);
	SceneManager::Load<Scene>(start_scene);

	while (TRUE)
	{

	MAIN_LOOP:
		try {
#ifdef DEBUG_WINDOW
			//=======================//
			//片方のウィンドウが消されたら、もう片方も終了する
			if (PeekMessage(&msg, window[0], 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			if (PeekMessage(&msg, GetMainWindowHandle(), 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
#endif

			//=======================//
			Time::Update();
			Input::Update();
#if 0
			ImGuiUpdate();
#endif
			//アップデート
			//GameUpdate();
			SceneManager::PreUpdate();
			SceneManager::Update();


			SceneManager::LateUpdate();
			SceneManager::PostUpdate();
			bool phys = false;
			double real_delta = Time::RealDeltaTimeD();
			double fixed_max = Time::GetFixedDeltaTimeMAXD();
			double max = (fixed_max > real_delta ? fixed_max : real_delta);
			//PCのスペック次第では実際に出ているFPSよりも物理更新を行おうとするので、
			//FPSが物理更新頻度を下回った場合はFPSを基準に計算頻度を決める
			while (Time::FixedDeltaTimeD() >= max)
			{
				//物理
				SceneManager::PrePhysics();
				SceneManager::Physics();
				SceneManager::PostPhysics();

				if (Time::FixedDeltaTimeD() >= Time::GetFixedDeltaTimeMAXD() * 2) {
					phys = true;

				}
				Time::FixFixedFPS();
			}
			//あまりよろしくはないが、FPS低下時にはPhysicsが暴走する可能性があるので、
			// fixed_deltatimeが2ループ分以上溜まったら時飛ばしを行う
			if (phys)
				Time::ResetTime();

			bool imgui_drawed = !(Time::DrawDeltaTimeD() >= Time::GetDrawDeltaTimeMAXD());
			//描画
			if (!imgui_drawed)
			{


				ClearDrawScreen();
				SceneManager::Draw();
				//GameRender();
#ifdef DEBUG_WINDOW
				//書き込みを行うウィンドウを、メインウィンドウに設定
				SetScreenFlipTargetWindow(NULL);
				ScreenFlip();
				//============//
				// メインウィンドウの映り込みがある場合は、直下の行を有効化
				//WaitTimer(2);
				ClearDrawScreen();
				//デバッグウィンドウへの描画
#endif
#ifdef USE_DEBUG_DRAW
				SceneManager::DebugDraw();
				SceneManager::LateDebugDraw();
#endif
#ifdef DEBUG_WINDOW
				//書き込みを行うウィンドウを、デバッグウィンドウに設定
				SetScreenFlipTargetWindow(window[0]);
#endif
#if 0
				ID3D11Texture2D* backBufferTex = reinterpret_cast<ID3D11Texture2D*>(const_cast<void*>(GetUseDirect3D11BackBufferTexture2D()));
				ID3D11ShaderResourceView* g_BackBufferSRV = nullptr;
				ID3D11Device* device = reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device())); // DxLibから取得

				D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
				srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MipLevels = 1;
				HRESULT hr = device->CreateShaderResourceView(backBufferTex, &srvDesc, &g_BackBufferSRV);
				if (FAILED(hr)) {
					// エラー処理
					PostQuitMessage(0);
				}
				else {
					//かっちょいいのやります。ImGuiの中で描画するンゴ
					ImGui::Begin("Game View", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
					auto size = ImGui::GetWindowSize();
					auto real_size = ImVec2(SCREEN_W, SCREEN_H);
					ImVec2 scale = size / real_size;
					scale = scale.x < scale.y ? ImVec2(scale.x, scale.x) : ImVec2(scale.y, scale.y);
					ImGui::Image((ImTextureID)g_BackBufferSRV, real_size * scale); // 解像度は画面サイズに応じて
					ImGui::End();
				}
#endif
				//本来のImGuiDrawの位置はここ
				//なんか最後に描画したものだけImGuiに描画領域が吸われるのでとりあえず画面外に何か書き込み
#if 0
				ImGuiDraw();
#endif
				//ImGuiの中でドローするなら、こっちは呼ばなくていい
				ScreenFlip();
				Time::FixDrawFPS();
				//============//
			}
#if 0
			else     //ImGuiだけは毎フレーム書かなきゃ怒られるので、強制的にドロー
				ImGuiDraw();
#endif
			//PostDrawする
			SceneManager::PostDraw();

			Time::FixFPS();
			Time::UpdateFPS();
			if (ProcessMessage())	break;
			if (Input::GetKey(KeyCode::Escape))	break;
		}
		catch (Exception& ex) {
			ex.Show();
			//ImGuiDraw();
			goto MAIN_LOOP;

		}

	}
#ifdef PACKAGE_BUILD
	std::quick_exit(0);
	ImGuiExit();
#endif
	//終了
	try {
		SceneManager::Exit();
	}
	catch (Exception& ex) {
		ex.Show();
	}

	try {
		PhysicsManager::Exit();
	}
	catch (Exception& ex) {
		ex.Show();
	}

	timeEndPeriod(1);
	DxLib::DxLib_End();
	std::ofstream f("data/tree.txt");
	if (!f.fail()) {


		auto& base_type = TypeInfo::Root();
		const TypeInfo* p = base_type.Child();
		bool            returnFromTraverse = false;
		const TypeInfo* next = nullptr;
		f << base_type.ClassName() << std::endl;
		//----------------------------------------------------------
		// 継承ツリー構造を探索
		// スタック再帰を使わない高速なツリー探索 (stackless tree traversal)
		//----------------------------------------------------------
		int nest = 0;
		while (p && (p != &base_type)) {
			if (!returnFromTraverse) {
				for (int i = 0; i < nest; i++)
					f << "|        " << std::flush;
				f << "|-----" << p->ClassName() << std::endl;
			}
			if (p->Child() && !returnFromTraverse) {
				// 子がある場合は子を先に調べる。(子から探索で戻ってきた場合は除外)
				nest++;
				next = p->Child();
				returnFromTraverse = false;
			}
			else if (p->Sibling()) {
				// 兄弟がいる場合は兄弟を調べる
				next = p->Sibling();
				returnFromTraverse = false;
			}
			else {
				// 親へ戻る。
				next = p->Parent();
				for (int i = 0; i < nest; i++)
					f << "|        " << std::flush;
				f << "end_of_node" << std::endl;
				for (int i = 0; i < nest; i++)
					f << "|        " << std::flush;
				f << std::endl;
				nest--;

				returnFromTraverse = true;
			}

			p = next;
		}
		f.close();
	}
	system("pause");
	return 0;
}


//---------------------------------------------------------------------------------
//	度をラジアンに変換する関数
//---------------------------------------------------------------------------------
float TO_RADIAN(float degree)
{
	return degree * 3.14159265f / 180.0f;
}
//---------------------------------------------------------------------------------
//	ラジアンを度に変換する関数
//---------------------------------------------------------------------------------
float TO_DEGREE(float radian)
{
	return radian * 180.0f / 3.14159265f;
}
//---------------------------------------------------------------------------------
//	ＸＺ方向に円を描画する
//---------------------------------------------------------------------------------
void DrawCircle3D_XZ(float3 center, float radius, int color, bool fill)
{
	VECTOR pos1;
	VECTOR pos2;
	for (int i = 0; i < 36; i++) {
		pos1.x = center.x + radius * sinf(TO_RADIAN(i * 10.0f));
		pos1.y = center.y;
		pos1.z = center.z + radius * cosf(TO_RADIAN(i * 10.0f));

		pos2.x = center.x + radius * sinf(TO_RADIAN((i + 1) * 10.0f));
		pos2.y = center.y;
		pos2.z = center.z + radius * cosf(TO_RADIAN((i + 1) * 10.0f));

		if (fill) {
			DrawTriangle3D(center, pos1, pos2, color, TRUE);
		}
		else {
			DrawLine3D(pos1, pos2, color);
		}
	}
}
//---------------------------------------------------------------------------------
//	ＸＺ方向に四角を描画する
//---------------------------------------------------------------------------------
void DrawBox3D_XZ(float3 center, float half_w, float half_h, int color, bool fill)
{
	VECTOR pos1, pos2, pos3, pos4;

	pos1.x = center.x + half_w;
	pos1.y = center.y;
	pos1.z = center.z + half_h;

	pos2.x = center.x + half_w;
	pos2.y = center.y;
	pos2.z = center.z - half_h;

	pos3.x = center.x - half_w;
	pos3.y = center.y;
	pos3.z = center.z - half_h;

	pos4.x = center.x - half_w;
	pos4.y = center.y;
	pos4.z = center.z + half_h;

	if (fill) {
		DrawTriangle3D(pos1, pos2, pos3, color, TRUE);
		DrawTriangle3D(pos1, pos3, pos4, color, TRUE);
	}
	else {
		DrawLine3D(pos1, pos2, color);
		DrawLine3D(pos2, pos3, color);
		DrawLine3D(pos3, pos4, color);
		DrawLine3D(pos4, pos1, color);
	}

}



int CreateDebugWindow(HINSTANCE& hInstance, HWND& window, int window_x, int window_y, WNDCLASS& window_parameter, int nCmdShow)
{
	//==================================//


// デバッグウインドウの作成
	window_parameter.style = CS_HREDRAW | CS_VREDRAW;
	window_parameter.lpfnWndProc = WndProc;
	window_parameter.cbClsExtra = 0;
	window_parameter.cbWndExtra = 0;
	window_parameter.hInstance = hInstance;
	window_parameter.hIcon = NULL;
	window_parameter.hCursor = LoadCursor(NULL, IDC_ARROW);
	window_parameter.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	window_parameter.lpszMenuName = NULL;
	window_parameter.lpszClassName = window_classname[0].c_str();

	if (!RegisterClass(&window_parameter))
	{
		return -1;
	}

	window = CreateWindow(
		window_classname[0].c_str(),
		"デバッグウィンドウ",
		WS_MINIMIZEBOX | WS_SYSMENU,
		window_x * 0.5f, window_y * 0.5f, window_x, window_y,
		NULL, NULL, hInstance, NULL
	);
	ShowWindow(window, nCmdShow);
	UpdateWindow(window);

	return 0;
	//==================================//
}
