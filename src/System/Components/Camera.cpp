#include "precompile.h"
#include "Camera.h"
#include "System/Scene.h"
#include <System/Utils/Render.h>
#include <System/Objects/ShadowMapObject.h>




namespace CreateMatrix {

	//---------------------------------------------------------------------------
	//!  [左手座標系] ビュー行列
	//---------------------------------------------------------------------------
	mat4x4 lookAtLH(const Vector3& eye, const Vector3& lookAt, const Vector3& worldUp)
	{
		Vector3 axis_z = (lookAt - eye).getNormalized();
		Vector3 axis_x = worldUp.cross(axis_z).getNormalized();
		Vector3 axis_y = axis_z.cross(axis_x);

		f32 tx = -axis_x.dot(eye);
		f32 ty = -axis_y.dot(eye);
		f32 tz = -axis_z.dot(eye);

		Vector4 m[4]{
			{ axis_x.x, axis_y.x, axis_z.x, 0.0f },
			{ axis_x.y, axis_y.y, axis_z.y, 0.0f },
			{ axis_x.z, axis_y.z, axis_z.z, 0.0f },
			{ tx,       ty,       tz, 1.0f },
		};
		return mat4x4(m[0], m[1], m[2], m[3]);
	}

	//---------------------------------------------------------------------------
	//! [左手座標系] 平行投影行列
	//---------------------------------------------------------------------------
	mat4x4 orthographicOffCenterLH(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z)
	{
		float rcp_width = 1.0f / (right - left);
		float rcp_height = 1.0f / (top - bottom);
		float range = 1.0f / (far_z - near_z);

		Vector4 m[4]{
			{ rcp_width * 2.0f,                         0.0f,            0.0f, 0.0f },
			{ 0.0f,            rcp_height * 2.0f,            0.0f, 0.0f },
			{ 0.0f,                         0.0f,           range, 0.0f },
			{ -(left + right) * rcp_width, -(top + bottom) * rcp_height, -range * near_z, 1.0f }
		};
		return mat4x4(m[0], m[1], m[2], m[3]);
	}

	//---------------------------------------------------------------------------
	//! [左手座標系] 投影行列
	//---------------------------------------------------------------------------
	mat4x4 perspectiveFovLH(f32 fovy, f32 aspect_ratio, f32 near_z, f32 far_z)
	{
		f32 s = std::sinf(fovy * 0.5f);
		f32 c = std::cosf(fovy * 0.5f);

		f32 height = c / s;
		f32 width = height / aspect_ratio;
		f32 range = far_z / (far_z - near_z);

		Vector4 m[4]{
			{ width,   0.0f,            0.0f, 0.0f },
			{ 0.0f, height,            0.0f, 0.0f },
			{ 0.0f,   0.0f,           range, 1.0f },
			{ 0.0f,   0.0f, -range * near_z, 0.0f }
		};
		return mat4x4(m[0], m[1], m[2], m[3]);
	}

}

struct CBufferCameraInfo {
	mat4x4 view;
	mat4x4  proj;
	Vector4 camera_pos;
	mat4x4 mat_light_view_proj_[4];    //!< ライトのビュー投影行列
};


void Camera::Construct()
{
	SetPriority(1);
	status.status_bit.on(CompStat::STATUS::SINGLE);
}

int Camera::Init()
{
	my_screen = TextureManager::Create(owner->name + "camerascreen", SCREEN_W, SCREEN_H, DXGI_FORMAT_R8G8B8A8_UNORM);
	my_screen_depth = TextureManager::Create(owner->name + "camerascreendepth", SCREEN_W, SCREEN_H, DXGI_FORMAT_D32_FLOAT);

	constant_buffer_handle = CreateShaderConstantBuffer(sizeof(CBufferCameraInfo));

	//if (!my_screen)
	//	my_screen = TextureManager::CloneByName(owner->name + "camerascreen");
	if (!GetCurrentCamera())
		SetCurrentCamera();
	return 0;
}

void Camera::PreDraw()
{
	PrepareCamera();
	if (is_current_camera)
	{
		ClearColor(my_screen.raw_shared().get(), { 0,0,0,0 });
		ClearDepth(my_screen_depth.raw_shared().get(), 1.0f);
	}
}

void Camera::PrepareCamera()
{
	auto owner_trns = owner->transform;
	if (is_current_camera)
	{
		SetCameraConstantBuffer();
		SetRenderTarget(my_screen.raw_shared().get(), my_screen_depth.raw_shared().get());
		SetupCamera_Perspective(DEG2RAD(perspective));
		SetCameraPositionAndTargetAndUpVec(cast(owner_trns->position), cast(owner_trns->position + owner_trns->AxisZ()), cast(owner_trns->AxisY()));
		SetCameraNearFar(camera_near, camera_far);
	}
}

void Camera::SetCameraConstantBuffer()
{
	if (!is_current_camera)
		return;
	void* p = GetBufferShaderConstantBuffer(constant_buffer_handle);
	if (!p)
		return;
	{
		auto owner_trns = owner->transform;
		auto* cb = reinterpret_cast<CBufferCameraInfo*>(p);
		cb->camera_pos = Vector4(owner_trns->position, 0);
		cb->view = CreateMatrix::lookAtLH(owner_trns->position, owner_trns->AxisZ(), Vector3(0, 1, 0));
		cb->proj = CreateMatrix::perspectiveFovLH(DEG2RAD(perspective), (float)SCREEN_W / (float)SCREEN_H, camera_near, camera_far);
		auto shadow_map = SceneManager::Object::Get<ShadowMapObject>(owner->GetScene());
		if (shadow_map) {
			auto light_view_projs = shadow_map->GetLightViewProjs();
			for (int i = 0; i < light_view_projs.size() && i < 4; i++)
				cb->mat_light_view_proj_[i] = light_view_projs[i];
		}

	}
	UpdateShaderConstantBuffer(constant_buffer_handle);
	SetShaderConstantBuffer(constant_buffer_handle, DX_SHADERTYPE_VERTEX, 11);
	SetShaderConstantBuffer(constant_buffer_handle, DX_SHADERTYPE_PIXEL, 11);
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

void DebugCamera::Construct()
{
	if (auto another = owner->GetScene()->GetDebugCamera().lock())
	{
		if (another.raw_shared().get() != this) {
			RemoveThisComponent();
			return;
		}
	}
	SetPriority(1);
	status.status_bit.on(CompStat::STATUS::SINGLE);
}

int DebugCamera::Init()
{
	my_screen = TextureManager::Create(owner->name + "d_camerascreen", SCREEN_W, SCREEN_H, DXGI_FORMAT_R8G8B8A8_UNORM);
	my_screen_depth = TextureManager::Create(owner->name + "d_camerascreendepth", SCREEN_W, SCREEN_H, DXGI_FORMAT_D32_FLOAT);

	return 0;
}

void DebugCamera::Update()
{
	auto parent = owner->transform->parent.lock();
	auto owner_trns = owner->transform;
	if (Input::GetKey(KeyCode::W))
		parent->MovePosition(owner_trns->AxisZ() * 1000 * Time::RealDeltaTime());
	if (Input::GetKey(KeyCode::S))
		parent->MovePosition(-owner_trns->AxisZ() * 1000 * Time::RealDeltaTime());
	if (Input::GetKey(KeyCode::A))
		parent->MovePosition(-owner_trns->AxisX() * 1000 * Time::RealDeltaTime());
	if (Input::GetKey(KeyCode::D))
		parent->MovePosition(owner_trns->AxisX() * 1000 * Time::RealDeltaTime());
	if (Input::GetKey(KeyCode::Q))
		parent->MovePosition(-owner_trns->AxisY() * 1000 * Time::RealDeltaTime());
	if (Input::GetKey(KeyCode::E))
		parent->MovePosition(owner_trns->AxisY() * 1000 * Time::RealDeltaTime());
	Vector2 mouse_move = Input::GetMouseDelta();
	Quaternion qx = Quaternion(mouse_move.x * Time::RealDeltaTime() * 0.2f, { 0,1,0 });
	Quaternion qy = Quaternion(mouse_move.y * Time::RealDeltaTime() * 0.2f, { 1,0,0 });

	if (Input::GetMouseButtonRepeat(MouseButton::ButtonRight)) {
		owner->transform->local_rotation = qy * owner->transform->local_rotation;
		parent->AddRotation(qx);
	}
}

void DebugCamera::PreDraw()
{
	PrepareCamera();
}


void DebugCamera::PrepareCamera()
{

	ClearColor(my_screen.raw_shared().get(), { 0,0,0,0 });
	ClearDepth(my_screen_depth.raw_shared().get(), 1.0f);
	TransformP owner_trns = owner->transform;
	SetRenderTarget(my_screen.raw_shared().get(), my_screen_depth.raw_shared().get());
	SetupCamera_Perspective(DEG2RAD(perspective));
	SetCameraPositionAndTargetAndUpVec(cast(owner_trns->position), cast(owner_trns->position + owner_trns->AxisZ()), cast(owner_trns->AxisY()));
	SetCameraNearFar(camera_near, camera_far);
}

void DebugCamera::Exit()
{
	owner->GetScene()->SetDebugCamera(nullptr);
	my_screen.reset();
	my_screen_depth.reset();
	auto targets = GetRenderTarget();
	//もし自分のレンダーターゲットがセットされていたらデフォルトに戻す
	if (targets.color_targets_[0]) {
		if (targets.color_targets_[0] == my_screen.raw_shared().get())
		{
			auto current = GetCurrentCamera();
			SetRenderTarget(current->my_screen.raw_shared().get(), current->my_screen_depth.raw_shared().get());
		}
	}

}
