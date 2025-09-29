#pragma once
#include "System/Scene.h"
namespace NetWorkTest {

	class NetWorkSampleScene :
		public Scene
	{
	public:
		USING_SUPER(NetWorkSampleScene);
		int Init() override;
		void Update() override;
		void Draw() override;
		void Exit() override;
		bool is_connected = false;
	};
};

