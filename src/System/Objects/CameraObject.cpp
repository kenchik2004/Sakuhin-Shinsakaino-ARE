#include "precompile.h"
#include "CameraObject.h"
#include "System/Components/Camera.h"

int CameraObject::Init()
{
	auto camera_ = AddComponent<Camera>();
	SetPriority(0);
	camera = camera_;
	camera->camera_near = 0.1f;
	camera->camera_far = 2000.0f;
	camera->perspective = 30.0f;
	camera->SetCurrentCamera();
	return 0;
}

void CameraObject::Update()
{
}

int DebugCameraObject::Init()
{
	auto child = SceneManager::Object::Create<GameObject>();
	child->transform->SetParent(transform);
	camera = child->AddComponent<DebugCamera>();
	main_camera = GetScene()->GetCurrentCamera();
	GetScene()->SetDebugCamera(SafeStaticCast<DebugCamera>(camera.lock()));
	transform->position = Vector3(0, 5, -10);
	SetPriority(0);
	return 0;
}

void DebugCameraObject::PostPhysics()
{
	if (main_camera) {
		// 定数バッファ等はメインカメラのものを使うので、描画前にメインカメラと同じ優先度にする
		// 優先度が同じなら登録順で描画されるので、DebugCameraが後に登録されていればDebugCameraの描画が優先される
		if (GetPriority() < main_camera->GetPriority())
			SetPriority(main_camera->GetPriority());
	}
}
