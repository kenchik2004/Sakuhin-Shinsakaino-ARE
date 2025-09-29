#pragma once
#include <System/Component.h>
class Button : public Component
{
	const Vector3 DEFAULT_BUTTON_SIZE = { 100, 50,1 };

public:
	USING_SUPER(Button);
	int          Init() override;
	void          Update() override;
	void          LateDraw() override;
	void          SetFunc(std::function<void()> func) { BtnFunc = func; }
	void          SetImage(std::string_view file_path);
	unsigned int& Color() { return button_color; }
	unsigned int& OnButtonColor() { return mouse_on_color; }
	unsigned int& PressedColor() { return pressed_color; }

	enum BUTTON_TEXTURE_TYPE
	{
		COLOR,
		IMAGE,
		NONE,
	};

	BUTTON_TEXTURE_TYPE& TextureType() { return texture_type; }

private:
	unsigned int              button_color = Color::WHITE;
	unsigned int              pressed_color = Color::RED;
	unsigned int              mouse_on_color = Color::GRAY;
	unsigned int              current_color = button_color;
	Vector3                    button_size = DEFAULT_BUTTON_SIZE;
	bool                      is_pressed = false;
	bool                      was_pressed = false;
	int                       image = -1;
	std::string               image_path = "";
	int                       null_image = -1;
	BUTTON_TEXTURE_TYPE       texture_type = COLOR;
	std::function<void(void)> BtnFunc = nullptr;
};
