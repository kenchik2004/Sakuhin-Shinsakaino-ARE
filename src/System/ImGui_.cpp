#include "ImGui_.h"
#include <../imgui/backends/imgui_impl_win32.h>
#include <../imgui/backends/imgui_impl_dx9.h>
#include <../imgui/backends/imgui_impl_dx11.h>

#include <shlobj.h>

// ImGui実装部
#define IMGUI_IMPLEMENTATION

#include <../imgui/misc/cpp/imgui_stdlib.h>

// メッセージハンドラ (imgui_impl_win32.cpp)
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

namespace {

	//===========================================================================
	// ImGuiバックエンドインターフェイス
	//===========================================================================
	class IBackend
	{
	public:
		IBackend() = default;
		virtual ~IBackend() = default;

		//! 初期化
		virtual bool initialize() = 0;

		//! フレーム開始
		virtual void newFrame() = 0;

		//! 描画
		virtual void renderDrawData(ImDrawData* draw_data) = 0;

		//! 終了
		virtual void shutdown() = 0;
	};

	//===========================================================================
	// ImGui DirectX9バックエンド
	//===========================================================================
	class BackendDX9 : public IBackend
	{
		//! 初期化
		virtual bool initialize() override
		{
			auto d3d_device = static_cast<IDirect3DDevice9*>(const_cast<void*>(GetUseDirect3DDevice9()));
			return ImGui_ImplDX9_Init(d3d_device);
		}

		//! フレーム開始
		virtual void newFrame() override { ImGui_ImplDX9_NewFrame(); }

		//! 描画
		virtual void renderDrawData(ImDrawData* draw_data) override { ImGui_ImplDX9_RenderDrawData(draw_data); }

		//! 終了
		virtual void shutdown() override { ImGui_ImplDX9_Shutdown(); }
	};

	//===========================================================================
	// ImGui DirectX11バックエンド
	//===========================================================================
	class BackendDX11 : public IBackend
	{
		//! 初期化
		virtual bool initialize() override
		{
			auto d3d_device = static_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
			auto d3d_context = static_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));
			return ImGui_ImplDX11_Init(d3d_device, d3d_context);
		}

		//! フレーム開始
		virtual void newFrame() override { ImGui_ImplDX11_NewFrame(); }

		//! 描画
		virtual void renderDrawData(ImDrawData* draw_data) override { ImGui_ImplDX11_RenderDrawData(draw_data); }

		//! 終了
		virtual void shutdown() override { ImGui_ImplDX11_Shutdown(); }
	};

	//! レンダリングバックエンド
	std::unique_ptr<IBackend> backend_;

	bool process_end = false;

}    // namespace

//! @brief DestroyWindowから処理終了フラグ
//! @return 処理終了している
bool IsProcEnd()
{
	return process_end;
}

//---------------------------------------------------------------------------------
//!	初期化
//---------------------------------------------------------------------------------
void ImGuiInit(bool use_keyboard)
{
	IMGUI_CHECKVERSION();

	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	if (use_keyboard)
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;    // キーボード操作
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // ドッキングウィンドウ
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;    // マルチウィンドウ

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		// マルチウィンドウの場合
		style.WindowRounding = 0.0f;    // ウィンドウの角を丸くしない
		//style.Colors[ImGuiCol_WindowBg].w = 1.0f;    // 半透明にしない
		style.Colors[ImGuiCol_WindowBg] =ImVec4(0.2f,0.2f,0.2f,1);

		style.WindowPadding = ImVec2(0, 0);     // ウィンドウ内のコンテンツパディング
		style.WindowBorderSize = 0.0f;          // ウィンドウの枠線の太さ
		style.FrameBorderSize = 0.0f;           // フレームの枠線の太さ
		}
	style.PopupRounding = 4.0f;    // ポップアップウィンドウの角を丸くする
	style.FrameRounding = 4.0f;    // 操作UIの縁を丸くする

	// DPI対応
	ImGui_ImplWin32_EnableDpiAwareness();
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleViewports;
	io.ConfigFlags |= ImGuiConfigFlags_DpiEnableScaleFonts;


	//----------------------------------------------------------
	// 日本語フォント対応
	//----------------------------------------------------------
	ImFontConfig config;
	config.MergeMode = true;

	// フォントパスを取得
	{
		CHAR font_path[MAX_PATH]{};
		SHGetSpecialFolderPath(nullptr, font_path, CSIDL_FONTS, 0);

		// 日本語 Japanese
		auto font_path_consolas = std::string(font_path) + "/consola.ttf";
		auto font_path_meiryo = std::string(font_path) + "/meiryo.ttc";
		io.Fonts->AddFontFromFileTTF(font_path_consolas.c_str(), 18.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
		io.Fonts->AddFontFromFileTTF(font_path_meiryo.c_str(), 20.0f, &config, io.Fonts->GetGlyphRangesJapanese());

		// 中国簡体字 Chinese
		//auto font_path_consolas = std::string(font_path) + "/consola.ttf";
		//auto font_path_yahei   = std::string(font_path) + "/msyh.ttc";
		//io.Fonts->AddFontFromFileTTF(font_path_consolas.c_str(), 12.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
		//io.Fonts->AddFontFromFileTTF(font_path_yahei.c_str(), 16.0f, &config, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());

		// 中国繁体字
		//auto font_path_consolas = std::string(font_path) + "/consola.ttf";
		//auto font_path_yahei   = std::string(font_path) + "/msyh.ttc";
		//io.Fonts->AddFontFromFileTTF(font_path_consolas.c_str(), 12.0f, nullptr, io.Fonts->GetGlyphRangesDefault());
		//io.Fonts->AddFontFromFileTTF(font_path_yahei.c_str(), 16.0f, &config, io.Fonts->GetGlyphRangesChineseFull());

		// ロシア語 Russian
		//auto font_path_tahoma = std::string(font_path) + "/tahomabd.ttf";
		//auto font_path_tahoma = std::string(font_path) + "/tahoma.ttf";
		//io.Fonts->AddFontFromFileTTF(font_path_tahoma.c_str(), 12.0f, nullptr, io.Fonts->GetGlyphRangesCyrillic());

		// ベトナム語 Vietnamese
		//auto font_path_tahoma = std::string(font_path) + "/tahomabd.ttf";
		//auto font_path_tahoma = std::string(font_path) + "/tahoma.ttf";
		//io.Fonts->AddFontFromFileTTF(font_path_tahoma.c_str(), 12.0f, nullptr, io.Fonts->GetGlyphRangesVietnamese());

		// タイ語 Thai
		//auto font_path_tahoma = std::string(font_path) + "/tahomabd.ttf";
		//auto font_path_tahoma = std::string(font_path) + "/tahoma.ttf";
		//io.Fonts->AddFontFromFileTTF(font_path_tahoma.c_str(), 12.0f, nullptr, io.Fonts->GetGlyphRangesThai());
	}

	//----------------------------------------------------------
	// DxLibカスタムのウィンドウプロシージャ
	//----------------------------------------------------------
	DxLib::SetHookWinProc([](HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT /*CALLBACK*/
		{
			switch (message) {
			case WM_DESTROY:
				process_end = true;
				DxLib::SetUseHookWinProcReturnValue(TRUE);
				PostQuitMessage(0);
				return 0;

			default:
				// DxLibとImGuiのウィンドウプロシージャを両立させる
				DxLib::SetUseHookWinProcReturnValue(FALSE);
				return ImGui_ImplWin32_WndProcHandler(hwnd, message, wparam, lparam);
			}
		});

	//----------------------------------------------------------
	// プラットフォームとレンダラのバックエンドを初期化
	//----------------------------------------------------------
	ImGui_ImplWin32_Init(DxLib::GetMainWindowHandle());

	// DxLibのD3Dバージョンによってバックエンドを選択
	switch (DxLib::GetUseDirect3DVersion()) {
	case DX_DIRECT3D_9:
	case DX_DIRECT3D_9EX:
		backend_ = std::make_unique<BackendDX9>();
		break;
	case DX_DIRECT3D_11:
		backend_ = std::make_unique<BackendDX11>();
		break;
	case DX_DIRECT3D_NONE:
	default:
		throw Exception("Can't get DxLib Direct3D Version", DEFAULT_EXCEPTION_PARAM);
	}
	// 初期化
	backend_->initialize();
}

//---------------------------------------------------------------------------------
//!	更新
//---------------------------------------------------------------------------------
void ImGuiUpdate()
{
	// フレーム開始
	backend_->newFrame();

	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	//----------------------------------------------------------
	// ImGuiサンプルデモ
	//----------------------------------------------------------
	if constexpr (false) {
		// 英語文字列以外はUTF-8で文字列を渡す必要がある
		ImGui::Begin("テスト");
		ImGui::Text("文字表示テスト");
		ImGui::End();
		// デモウィンドウ
		ImGui::ShowDemoWindow();
	}

}

//---------------------------------------------------------------------------------
//!	描画
//---------------------------------------------------------------------------------
void ImGuiDraw()
{
	// フレーム終了
	ImGui::EndFrame();

	// 描画データー生成
	ImGui::Render();

	// 描画
	backend_->renderDrawData(ImGui::GetDrawData());

	// Windowsの場合の追加の描画関数
	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
	}

	// 外部ライブラリの描画APIを呼んだ後にDxLibの関数を呼ぶ前に実行する必要あり
	RefreshDxLibDirect3DSetting();
}

//---------------------------------------------------------------------------------
//!	終了
//---------------------------------------------------------------------------------
void ImGuiExit()
{
	// 終了
	backend_->shutdown();
	backend_.reset();

	ImGui::DestroyContext();
}
