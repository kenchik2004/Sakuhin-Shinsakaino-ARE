#pragma once

namespace RLyeh {

	USING_PTR(DiceBase);
	class DicePair
	{
	public:
		USING_SUPER(DicePair);
		enum MODE {
			NONE,
			D4,
			D6,
			D8,
			D10,
			D12,
			D20,
			D100
		};
		MODE mode = NONE;
		DicePair() = default;
		DicePair(MODE mode_, DiceBaseP x1, DiceBaseP x2 = nullptr);
		int Result();
		std::array<DiceBaseP, 2> pair = { nullptr,nullptr };
	};
}

