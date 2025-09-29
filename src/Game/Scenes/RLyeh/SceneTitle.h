#pragma once
#include "System/Scene.h"
namespace RLyeh {
	USING_PTR(DiceScene);

	class SceneTitle :
		public Scene
	{
	public:
		USING_SUPER(SceneTitle);
		void Load() override;
		int Init()override;
		void Update() override;
		void LateDraw() override;
		void UnLoad() override;
		SafeSharedPtr<AudioClip> bgm = nullptr;
		DiceSceneWP dice_scene;
		SafeSharedPtr<Texture> title_img = nullptr;
		std::vector<int> results;
		CameraWP cam;
	};
}

