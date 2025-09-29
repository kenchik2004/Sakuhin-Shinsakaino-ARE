#pragma once
#include "System/Components/Text.h"
USING_PTR(Text);
class TextObject :
	public UIObject
{
public:
	USING_SUPER(TextObject);
	int Init() override;
	TextWP my_text;

};

