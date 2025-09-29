#pragma once
namespace NetWorkTest {

	class NetWorkSampleScene_Server :
		public Scene
	{
	public:
		USING_SUPER(NetWorkSampleScene_Server);
		int Init() override;
		void Update() override;
		void Draw() override;
		void Exit() override;
		bool is_connected = false;
	};
};

