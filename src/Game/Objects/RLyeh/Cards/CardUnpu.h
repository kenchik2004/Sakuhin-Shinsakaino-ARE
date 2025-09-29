#pragma once
#include "Game/Objects/RLyeh/Cards/CardBase.h"
namespace RLyeh {

	class CardUnpu :
		public CardBase
	{
	public:
		USING_SUPER(CardUnpu);
		int Init() override;
		void Exit() override;
	};

}
