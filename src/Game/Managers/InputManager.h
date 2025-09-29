#pragma once
#include "System/Object.h"
namespace RLyeh {

	class InputManager :
		public GameObject
	{
		InputManager();
		USING_SUPER(InputManager);
		int Init() override;

	};
}

