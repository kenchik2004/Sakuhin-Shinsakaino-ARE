#pragma once
#include "System/Component.h"

USING_PTR(Camera);
class Camera :
	public Component
{
public:
	void Construct() override;
	int Init() override;
	void PreDraw() override;
	void PrepareCamera();
	void SetCurrentCamera();
	void Exit() override { my_screen.reset(); }
	inline void SetPerspective(float perspective_) { perspective = perspective_; SetupCamera_Perspective(DEG2RAD(perspective)); }
	inline float GetPerspective() { return perspective; }
	SafeSharedPtr<Camera> GetCurrentCamera();
	bool is_current_camera = false;
	SafeSharedPtr<Texture> my_screen = nullptr;
	float camera_near = 0.1f;
	float camera_far = 1000.0f;
	float perspective = 45.0f;
};

