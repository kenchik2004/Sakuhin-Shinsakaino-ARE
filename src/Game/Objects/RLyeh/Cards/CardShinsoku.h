#pragma once
#include "Game/Objects/RLyeh/Cards/CardBase.h"
namespace RLyeh {

	class CardShinsoku :
		public CardBase
	{
	public:
		USING_SUPER(CardShinsoku);
		int Init() override;
		void Exit() override;
	};

}
