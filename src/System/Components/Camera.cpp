#include "precompile.h"
#include "Camera.h"
#include "System/Scene.h"

void Camera::Construct()
{
	SetPriority(1);
	status.status_bit.on(CompStat::STATUS::SINGLE);
}

int Camera::Init()
{
	my_screen = TextureManager::Create(owner->name + "camerascreen", SCREEN_W, SCREEN_H);
	if (!my_screen)
		my_screen = TextureManager::CloneByName(owner->name + "camerascreen");
	if (!GetCurrentCamera())
		SetCurrentCamera();
	return 0;
}

void Camera::PreDraw()
{
	if (is_current_camera)
	{
		int a=SetDrawScreen(my_screen->GetHandle());
		ClearDrawScreen();
	}
	PrepareCamera();
}

void Camera::PrepareCamera()
{
	auto owner_trns = owner->transform;
	if (is_current_camera)
	{
		SetDrawScreen(my_screen->GetHandle());
		SetupCamera_Perspective(perspective);
		SetCameraPositionAndTargetAndUpVec(cast(owner_trns->position), cast(owner_trns->position + owner_trns->AxisZ()), cast(owner_trns->AxisY()));
		SetCameraNearFar(camera_near, camera_far);
	}
}


void Camera::SetCurrentCamera()
{
	if (GetCurrentCamera())
		GetCurrentCamera()->is_current_camera = false;
	if (!owner->GetScene())
		return;
	owner->GetScene()->SetCurrentCamera(std::static_pointer_cast<Camera>(shared_from_this()));
	is_current_camera = true;
	PrepareCamera();
}

SafeSharedPtr<Camera> Camera::GetCurrentCamera()
{
	if (!owner->GetScene())
		return nullptr;
	return SafeStaticCast<Camera>(owner->GetScene()->GetCurrentCamera().lock());
}
