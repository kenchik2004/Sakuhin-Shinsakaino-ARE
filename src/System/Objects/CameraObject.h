#pragma once
#include "System/Object.h"

class CameraObject :
    public GameObject
{
public:
	USING_SUPER(CameraObject);
	int Init() override;
	void Update() override;
	CameraWP camera; // カメラコンポーネント
};
class DebugCameraObject :
	public GameObject
{
public:
	USING_SUPER(DebugCameraObject);
	int Init() override;
	void PostPhysics() override;
	DebugCameraWP camera; // カメラコンポーネント
	CameraWP main_camera; // メインカメラ
};
