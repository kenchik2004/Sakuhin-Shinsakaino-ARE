#pragma once
#include "System/Component.h"
class ShadowMap :
	public Component
{
public:
	USING_SUPER(ShadowMap);
	int Init() override;
	void Update() override;
	void PreDraw() override;
};

