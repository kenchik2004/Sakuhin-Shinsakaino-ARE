#pragma once
#include "System/Scene.h"
namespace SampleAnimation {
	class SampleAnimationScene :
		public Scene
	{
	public:
		USING_SUPER(SampleAnimationScene);
		void Load() override;
		int Init() override;
		void Update() override;
		void Draw() override;
		void LateDraw() override;
		void Exit() override;

	private:
		bool CheckForLoading(); //!< ロード状況の確認
		GameObjectWP camera = nullptr;
		ObjectWP player = nullptr;
		ObjectWP player2 = nullptr;
		ObjectWP player3 = nullptr;
	};

}
