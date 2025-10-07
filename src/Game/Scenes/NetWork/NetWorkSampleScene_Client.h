#pragma once
#include "System/Scene.h"
namespace NetWorkTest_Client {

	class NetWorkSampleScene_Client :
		public Scene
	{
	public:
		USING_SUPER(NetWorkSampleScene_Client);
		int Init() override;
		void Update() override;
		void LateDraw() override;
		void Exit() override;
		bool is_connected = false;
	};
};

