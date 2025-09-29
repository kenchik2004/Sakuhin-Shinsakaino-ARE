#pragma once
#include "System/DontDestroyOnLoadScene.h"
#include "Game/Utilitys/DicePair.h"
namespace RLyeh {

	USING_PTR(DiceBase);
	class DiceScene :
		public DontDestroyOnLoadScene
	{
	public:
		USING_SUPER(DiceScene);
		int Init();
		void Load() override;
		void Roll(int first, int second, bool use_dices = true);
		void Skip();
		void Clear();
		std::vector<int> FetchResults();
		std::vector<int> results_out_of_dices;

		void Exit() override;

		void Draw() override;
		void OnDrawFinish() override;
		void UnLoad() override;
		GameObjectWP camera;
		int texture[6] = { 0 };
		int tray_texture = -1;
		std::vector<DicePair> dices = std::vector<DicePair>(0);
	};


}