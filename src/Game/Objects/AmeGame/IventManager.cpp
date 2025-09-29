#include "precompile.h"
#include "IventManager.h"
#include "Game/Objects/AmeGame/Ivent.h"
#include "System/IniFileManager.h"


namespace AmeGame {
	IventWP IventManager::GetIvent(size_t index)
	{
		if (index < 0 || index >= ivents.size())
			return nullptr;
		return ivents[index];
	}
	void IventManager::RegisterIvents(std::vector<IventStartAndEnd> info) {
		for (auto&& info_ : info) {
			auto ivent_ = SceneManager::Object::Create<Ivent>();
			for (auto& ivent_text : info_.ivent_texts)
				ivent_->text_data.push_back(ivent_text);
			ivent_->transform->SetParent(transform);
			ivents.push_back(ivent_);

		}


	}
	void IventManager::IventStart(int index) {
		if (is_on_ivent || all_ivents_ended || index >= ivents.size())
			return;
		if (index >= 0)
			current_ivent = index;
		is_on_ivent = true;
		ivents[current_ivent]->status.status_bit.on(ObjStat::STATUS::ACTIVE);
		ivents[current_ivent]->status.status_bit.on(ObjStat::STATUS::DRAW);
	}
	void IventManager::IventEnd() {
		if (all_ivents_ended)
			return;
		ivents[current_ivent]->status.status_bit.off(ObjStat::STATUS::ACTIVE);
		ivents[current_ivent]->status.status_bit.off(ObjStat::STATUS::DRAW);
		is_on_ivent = false;
		current_ivent++;
		if (current_ivent >= ivents.size())
			all_ivents_ended = true;

	}
}