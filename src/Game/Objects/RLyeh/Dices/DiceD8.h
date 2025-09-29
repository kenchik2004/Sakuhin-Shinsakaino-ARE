#pragma once
#include "Game/Objects/RLyeh/Dices/DiceBase.h"

namespace RLyeh {

	class DiceD8 :
		public DiceBase
	{
	public:
		USING_SUPER(DiceD8);
		DiceD8(int texture);
	};

}
