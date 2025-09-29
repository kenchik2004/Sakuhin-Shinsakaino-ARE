#pragma once
#include "System/Object.h"
USING_PTR(Text);
namespace AmeGame {

	USING_PTR(Ivent);
	class Ivent :public GameObject {
	public:
		USING_SUPER(Ivent);
		TextWP ivent_text;
		int current_text = -1;
		std::function<void()> on_ivent_start = nullptr;
		std::function<void()> on_ivent_finish = nullptr;
		std::vector<std::string> text_data;
		int Init() override;
		void Update() override;
	};

}
