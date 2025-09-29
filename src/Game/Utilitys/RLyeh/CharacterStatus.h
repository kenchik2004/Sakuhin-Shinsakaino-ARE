#pragma once


class CharacterStatus
{
public:
	USING_SUPER(CharacterStatus);
	struct Parametor {
		unsigned short STR = 0;
		unsigned short CON = 0;
		unsigned short POW = 0;
		unsigned short DEX = 0;
		unsigned short APP = 0;
		unsigned short SIZ = 0;
		unsigned short INT = 0;
		unsigned short EDU = 0;
	};
	struct DamageBonus {
		std::string format = "0";
		//std::stringのフォーマット(1D4など)から、
		// ランダム整数を生成し、返す必要がある
	};
	struct Status {
		int HP = 0;
		int MP = 0;
		unsigned char SAN = 0;
		unsigned char IDEA = 0;
		unsigned char LUCK = 0;
		unsigned char KNOW = 0;


	};
	Parametor main_parametor;
	Status main_status;

};

