#pragma once
#include "System/Scene.h"
#include "System/Shader.h"

USING_PTR(Camera);
class SceneShader :
	public Scene
{
	USING_SUPER(SceneShader);
	int Init();
	void Update();
	void PreDraw() override;
	void Draw() override;
	void Exit() override;
	std::shared_ptr<ShaderPs> light_shader_ps = nullptr;
	std::shared_ptr<ShaderVs> light_shader_vs = nullptr;
	std::shared_ptr<ShaderPs> shader_ps = nullptr;
	std::shared_ptr<ShaderVs> shader_vs = nullptr;
	ObjectWP obj;
	ObjectWP model_obj;
	CameraWP camera;

	int model[2];
	SafeSharedPtr<Texture> shadow_map = nullptr;
	ID3D11SamplerState* d3d_shadow_sampler = nullptr;
};

mat4x4 lookAtLH(const Vector3& eye, const Vector3& lookAt, const Vector3& worldUp);

mat4x4 orthographicOffCenterLH(f32 left, f32 right, f32 bottom, f32 top, f32 near_z, f32 far_z);

mat4x4 perspectiveFovLH(f32 fovy, f32 aspect_ratio, f32 near_z, f32 far_z);
