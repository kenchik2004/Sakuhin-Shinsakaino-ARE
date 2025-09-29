#pragma once
#include "System/Object.h"
USING_PTR(Collider);
namespace AmeGame {
	USING_PTR(IventManager);
	class AmePlayer :
		public GameObject
	{
	public:
		USING_SUPER(AmePlayer);
		int Init() override;
		void Update() override;
		void OnTriggerEnter(const HitInfo& hit_info) override;
		ColliderWP foot;
		bool is_jumping = false;
	};

}
