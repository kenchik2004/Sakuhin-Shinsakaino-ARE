#include "Precompile.h"
#include <System/Components/Button.h>

int Button::Init()
{
	if (owner->status.Type() != ObjStat::UI)
		RemoveThisComponent();
	button_size = { owner->transform->scale };
	null_image = LoadGraph("data/Game/Image/nullptr.png");
	SetPriority(2);
	return Super::Init();
}

void Button::Update()
{
	auto owner_ = SafeStaticCast<UIObject>(owner.lock());
	button_size = owner->transform->scale;
	Vector3 draw_pos = owner_->GetDrawPos();
	owner_.reset();
	Vector3 end_pos = draw_pos + button_size;

	Vector2 mouse_pos = Input::GetMousePosition();

	bool is_mouse_on_button = (mouse_pos.x > draw_pos.x && mouse_pos.y > draw_pos.y && mouse_pos.x < end_pos.x && mouse_pos.y < end_pos.y);
	current_color = button_color;
	if (is_mouse_on_button) {
		current_color = mouse_on_color;
		is_pressed = Input::GetMouseButtonDown(MouseButton::ButtonLeft);

		//TODO
		//      関数ポインタに登録してある関数を呼び出す
		if (is_pressed) {
			current_color = pressed_color;
			//! コールバックの中には、シーン破棄やオブジェクト破棄など、
			//! 更新の途中で挟んではマズいものも登録される可能性があるので、
			//! 更新終了後に実行予約をする
			if (BtnFunc)
				SceneManager::func_on_loop_finish.push_back(BtnFunc);
		}
	}
}

void Button::LateDraw()
{
	auto   owner_ = SafeStaticCast<UIObject>(owner.lock());
	Vector3 draw_pos = owner_->GetDrawPos();
	Vector3 end_pos = draw_pos + button_size;

	switch (texture_type) {
	case Button::COLOR:
		DrawBoxAA(draw_pos.x, draw_pos.y, end_pos.x, end_pos.y, current_color, true);
		break;
	case Button::IMAGE:
		if (image > 0)
			DrawExtendGraphF(draw_pos.x, draw_pos.y, end_pos.x, end_pos.y, image, true);
		else
			DrawExtendGraphF(draw_pos.x, draw_pos.y, end_pos.x, end_pos.y, null_image, true);

		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
		DrawBoxAA(draw_pos.x, draw_pos.y, end_pos.x, end_pos.y, current_color, true);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
		break;
	default:
		break;
	}
}

void Button::SetImage(std::string_view file_path)
{
	image_path = file_path;
	image = LoadGraph(image_path.data());
}
