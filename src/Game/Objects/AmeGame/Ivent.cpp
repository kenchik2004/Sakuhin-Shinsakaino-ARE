#include "precompile.h"
#include "Ivent.h"
#include <System/Components/Text.h>
#include "Game/Scenes/AmeGame/SceneAme.h"
#include "Game/Objects/AmeGame/IventManager.h"

namespace AmeGame {
	int Ivent::Init() {
		ivent_text = SceneManager::GetScene<SceneAme>()->txtnavi->GetComponent<Text>();
		SetPriority(50);
		status.status_bit.off(ObjStat::STATUS::ACTIVE);
		status.status_bit.off(ObjStat::STATUS::DRAW);
		return 0;
	}
	void Ivent::Update() {
		if (current_text < 0)
		{
			if (on_ivent_start)
				on_ivent_start();
			current_text++;

			ivent_text->FontSize() = 28;
			ivent_text->text_speed = 30;
			ivent_text->SetFont("");
			ivent_text->SetText(text_data[current_text]);
			ivent_text->ResetDrawChar();
			return;
		}
		if (Input::GetKeyDown(KeyCode::Return))
		{
			current_text++;
			if (current_text >= text_data.size()) {
				current_text = -1;
				ivent_text->SetText("");
				SceneManager::Object::Get<IventManager>()->IventEnd();
				if (on_ivent_finish)
					on_ivent_finish();
				return;
			}
			ivent_text->FontSize() = 28;
			ivent_text->text_speed = 30;
			ivent_text->SetFont("");
			ivent_text->SetText(text_data[current_text]);
			ivent_text->ResetDrawChar();
		}
	}
}