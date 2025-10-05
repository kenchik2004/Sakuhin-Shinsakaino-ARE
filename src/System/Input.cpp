#include "precompile.h"
#include "Input.h"

namespace Input {

	char key_buffer[256];
	char key_buffer_prev[256];
	unsigned int mouse_buffer;
	unsigned int mouse_buffer_prev;

	Vector2 mouse_pos(0.0f, 0.0f);
	Vector2 mouse_pos_prev(0.0f, 0.0f);


	void Init()
	{
		for (int i = 0; i < 256; i++) {
			key_buffer[i] = 0;
			key_buffer_prev[i] = 0;
		}
		mouse_buffer = 0;
		mouse_buffer_prev = 0;

	}

	void Update()
	{

		mouse_pos_prev = mouse_pos;
		mouse_pos = GetMousePosition();
		memcpy(key_buffer_prev, key_buffer, sizeof(char) * 256);
		mouse_buffer_prev = mouse_buffer;

		GetHitKeyStateAll(key_buffer);
		mouse_buffer = 0;
		mouse_buffer |= GetMouseInput();



	}

	//---------------------------------------------------------------------------------
	//	キーが押された瞬間を取得する
	//---------------------------------------------------------------------------------
	bool GetKeyDown(KeyCode key)
	{
		return key_buffer[(unsigned char)key] == 1 && key_buffer_prev[(unsigned char)key] == 0;
	}
	//---------------------------------------------------------------------------------
	//	キーが押されているかどうかを取得する
	//---------------------------------------------------------------------------------

	bool GetKey(KeyCode key)
	{
		return key_buffer[(unsigned char)key] == 1 && key_buffer_prev[(unsigned char)key] == 1;
	}

	//---------------------------------------------------------------------------------
	//	キーが離された瞬間を取得する
	//---------------------------------------------------------------------------------
	bool GetKeyUp(KeyCode key) {
		return key_buffer[(unsigned char)key] == 0 && key_buffer_prev[(unsigned char)key] == 1;
	}
	//---------------------------------------------------------------------------------
	//	マウスが押されているかを取得する
	//---------------------------------------------------------------------------------
	bool CheckMouseInput(int button)
	{
		if (GetMouseInput() & button) {
			return true;
		}
		return false;
	}
	//---------------------------------------------------------------------------------
	//	マウスが押された瞬間を取得する
	//---------------------------------------------------------------------------------
	bool GetMouseButtonDown(MouseButton button)
	{
		return mouse_buffer & (unsigned char)button & (~mouse_buffer_prev);
	}
	bool GetMouseButtonRepeat(MouseButton button)
	{
		return mouse_buffer_prev & mouse_buffer & (unsigned char)button;
	}
	bool GetMouseButtonUp(MouseButton button)
	{
		return mouse_buffer_prev & (~mouse_buffer) & (unsigned char)button;
	}
	//---------------------------------------------------------------------------------
	//	マウスの座標を取得する
	//---------------------------------------------------------------------------------

	Vector2 GetMousePosition()
	{
		int mouse_x;
		int mouse_y;
		GetMousePoint(&mouse_x, &mouse_y);
		return Vector2(static_cast<float>(mouse_x), static_cast<float>(mouse_y));
	}

	void SetMousePosition(Vector2 new_pos, bool reset_delta) {
		SetMousePoint(static_cast<int>(new_pos.x), static_cast<int>(new_pos.y));
		mouse_pos = GetMousePosition();
		if (reset_delta)
			mouse_pos_prev = mouse_pos;
	}

	//---------------------------------------------------------------------------------
	//	マウスの移動量を取得する
	//---------------------------------------------------------------------------------
	Vector2 GetMouseDelta()
	{
		return mouse_pos - mouse_pos_prev;
	}

}