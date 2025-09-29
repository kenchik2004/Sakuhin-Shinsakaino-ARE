#pragma once
#include "System/Object.h"
#include "Game/Objects/AmeGame/Ivent.h"
namespace AmeGame {
	USING_PTR(Ivent);
	class IventManager :public GameObject {
	public:
		USING_SUPER(IventManager);
		size_t current_ivent = 0;
		bool is_on_ivent = false;
		bool all_ivents_ended = false;
		IventWPVec ivents;
		struct IventStartAndEnd {
			std::vector<std::string> ivent_texts;
		};
		IventWP GetIvent(size_t index);
		void RegisterIvents(std::vector<IventStartAndEnd> info);
		void IventStart(int index = -1);
		void IventEnd();

	};
}

