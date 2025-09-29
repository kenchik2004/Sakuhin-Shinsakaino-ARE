#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS
#include <../imgui/imgui.h>
// ImGui


//	初期化
void ImGuiInit(bool use_keyboard = false);

//	更新
void ImGuiUpdate();

//	描画
void ImGuiDraw();

//	終了
void ImGuiExit();

// Destroyが早くリーク情報が出せないため、WindowをDestroyを遅らせる
bool IsProcEnd();
