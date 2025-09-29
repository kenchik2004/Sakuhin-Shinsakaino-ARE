#pragma once
#include "Game/Objects/RLyeh/Dices/DiceBase.h"
namespace RLyeh {

	class DiceD6 :
		public DiceBase
	{
	public:
		USING_SUPER(DiceD6);
		DiceD6(int texture);
	};


}