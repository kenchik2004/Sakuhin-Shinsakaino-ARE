#pragma once
#include "Game/Objects/RLyeh/Dices/DiceBase.h"

namespace RLyeh {

	class DiceD12 :
		public DiceBase
	{
	public:
		USING_SUPER(DiceD12);
		DiceD12(int texture);
	};

}
