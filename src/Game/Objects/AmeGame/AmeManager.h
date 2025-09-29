#pragma once
#include "System/Object.h"
namespace AmeGame {
	struct Ame {
		Vector3 position;
		Vector3 hit_position = { -100,-100,-100 };
		Vector3 dir = { 0,-1,0 };
		float reflect_timer = 0;
	};
	class AmeManager :
		public GameObject
	{
	public:
		USING_SUPER(AmeManager);
		int Init() override;
		void Update() override;
		void Draw() override;
		GameObjectWP camera;

	private:
		std::array<Ame, 1000> ame = {};
	};
}

