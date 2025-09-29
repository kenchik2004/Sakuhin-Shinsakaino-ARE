#pragma once
#include "System/Object.h"

class CameraObject :
    public GameObject
{
public:
	USING_SUPER(CameraObject);
	int Init() override;
	CameraWP camera; // カメラコンポーネント
};

