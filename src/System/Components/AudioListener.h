#pragma once
#include "System/Component.h"

USING_PTR(AudioListener);
class AudioListener final :
	public Component
{
	bool is_current_listener = false;
public:
	void Construct() override;
	int Init() override;
	void PreDraw() override;
	void SetCurrentListener();
	SafeSharedPtr<AudioListener> GetCurrentListener();
};

