#include "precompile.h"
#include "InputManager.h"

namespace RLyeh {
	InputManager::InputManager() :GameObject()
	{
		tag = TAG::GameManager;
		name = "InputManager";
	}
	int InputManager::Init()
	{
		//こいつはデフォルト裏シーンに一匹だけ置いておく
		if (GetScene() != SceneManager::GetDontDestoryOnLoadScene())
			SceneManager::Object::DontDestroyOnLoad(shared_from_this(), GetScene());
		return 0;
	}
}