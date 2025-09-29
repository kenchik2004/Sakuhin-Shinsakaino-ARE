#pragma once
#include "System/Object.h"

namespace RLyeh {

	class DiceBase :
		public GameObject
	{
	public:
		USING_SUPER(DiceBase);
		DiceBase() = default;
		DiceBase(int handle_) :handle(handle_) {}
		int Init()override;
		void FetchResult();
		void DebugDraw();
		void LateDebugDraw();
		std::vector<Vector3> dice_vectors;
		int handle = -1;
		int selected_number = 0;
		std::string model_name = "";
	};
}

