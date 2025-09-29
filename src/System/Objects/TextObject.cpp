#include "precompile.h"
#include "TextObject.h"

int TextObject::Init()
{

	my_text = AddComponent<Text>();
	my_text->SetText("START!");
	my_text->TextColor() = Color::BLACK;
	return Super::Init();
}
