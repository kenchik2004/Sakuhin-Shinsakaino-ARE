#pragma once

#define SET_FUNCTION(FUNC) [this]() { FUNC(); }
#include <System/Components/Button.h>
USING_PTR(Button);

class ButtonObject : public UIObject
{
public:
	USING_SUPER(ButtonObject);
	int Init() override;
	void Update() override;
};
