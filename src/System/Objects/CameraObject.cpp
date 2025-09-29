#include "precompile.h"
#include "CameraObject.h"
#include "System/Components/Camera.h"

int CameraObject::Init()
{
	auto camera_ = AddComponent<Camera>();
	camera = camera_;
	camera->camera_near = 0.1f;
	camera->camera_far = 1000.0f;
	camera->perspective = 90.0f;
	camera->SetCurrentCamera();
	return 0;
}
