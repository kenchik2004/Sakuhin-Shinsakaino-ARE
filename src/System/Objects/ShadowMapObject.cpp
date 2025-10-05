#include "precompile.h"
#include "ShadowMapObject.h"
#include "System/Components/Camera.h"
#include "System/Utils/Render.h"
#include "System/Components/ModelRenderer.h"



namespace ShadowMapHelper {
	// 2点間の距離を求める
	float distance(const Vector3& a, const Vector3& b)
	{
		Vector3 d = a - b;

		float lengthSq = d.magnitudeSquared();
		if (lengthSq < 0.000001f)
			return 0.0f;

		return sqrtf(lengthSq);
	}

	// 8点から外接球（バウンディング球）を求める
	void ComputeBoundingSphere(const Vector3* points, u32 count, Vector3& outCenter, float& outRadius)
	{
		// (1) 8点の重心を初期中心とする
		Vector3 center = Vector3(0.0f, 0.0f, 0.0f);
		for (u32 i = 0; i < count; ++i) {
			center += points[i];
		}
		center /= float(count);

		// (2) 最も遠い点までの距離を半径とする
		float radius = 0.0f;
		for (u32 i = 0; i < count; ++i) {
			float d = distance(center, points[i]);
			if (d > radius)
				radius = d;
		}

		// (3) 収束まで中心を調整（数回繰り返す）
		if (0)
			for (u32 iter = 0; iter < 8; ++iter) {
				int   farthest = 0;
				float maxDist = 0.0f;
				for (u32 i = 0; i < count; ++i) {
					float d = distance(center, points[i]);
					if (d > maxDist) {
						maxDist = d;
						farthest = i;
					}
				}
				if (maxDist <= radius + 1e-5f)
					break;
				Vector3 dir = (points[farthest] - center).getNormalized();
				center += dir * (maxDist - radius) * 0.5f;
				radius = (radius + maxDist) * 0.5f;
			}

		outCenter = center;
		outRadius = radius;
	}
}



int ShadowMapObject::Init()
{
	SetPriority(UINT_MAX); // 最後に描画されるようにする

	return 0;
}

void ShadowMapObject::PreDraw()
{
	// シャドウマップの初期化(サイズを指定したいので遅延初期化とする)
	if (!is_initialized)
		//初期化に失敗したら以降の処理をしない
		if (!InitializeShadowMap())
			return;
	ShadowMapDrawBegin();
	ShadowMapDrawEnd();
	auto context = GetD3DDeviceContext();
	ID3D11ShaderResourceView* srv = shadow_map->Srv();
	context->PSSetShaderResources(13, 1, &srv);

}

void ShadowMapObject::LateDraw()
{
	DxLib::SetUseTextureToShader(13, -1);
	//for(auto& info: shadow_infos)
	//	info.draw();
	DrawLine3D(cast(shadowmap_center), cast(shadowmap_center + light_dir * 1), 0xffff00);
}

void ShadowMapObject::Exit()
{
	model_renderers.clear();
	shadow_map.reset();
	if (shadow_sampler) {
		shadow_sampler->Release();
		shadow_sampler = nullptr;
	}
	shadow_infos.clear();
}

void ShadowMapObject::RegisterModelRenderer(ModelRenderer* renderer)
{
	if (!renderer)
		return;
	if (std::find(model_renderers.begin(), model_renderers.end(), renderer) == model_renderers.end())
		model_renderers.push_back(renderer);
}

void ShadowMapObject::UnregisterModelRenderer(ModelRenderer* renderer)
{
	if (!renderer)
		return;
	auto ite = std::find(model_renderers.begin(), model_renderers.end(), renderer);
	if (ite != model_renderers.end())
		model_renderers.erase(ite);
}

bool ShadowMapObject::InitializeShadowMap()
{
	if (is_initialized)
		return true;
	shadow_map = make_safe_shared<Texture>(shadow_map_size * cascade_count, shadow_map_size, DXGI_FORMAT_D32_FLOAT);

	if (!shadow_map)
		return false;

	//シャドウマップのサンプリング品質を上げるためにComparison Samplerを作成する
	{
		ID3D11Device* d3d_device = reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
		D3D11_SAMPLER_DESC desc{
				   .Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // ピクセルサンプリング動作モード
				   .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP, // テクスチャのアドレスモードU
				   .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP, // テクスチャのアドレスモードV
				   .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP, // テクスチャのアドレスモードW
				   .MipLODBias = 0.0f, // MIPレベルのバイアス値
				   .MaxAnisotropy = 1, // 異方性サンプリングの品質 [1,16]
				   .ComparisonFunc = D3D11_COMPARISON_LESS, // 数値比較の場合の大小関係
				   .BorderColor = {1.0f, 1.0f, 1.0f, 1.0f}, // 範囲外の色(ボーダーカラー)
				   .MinLOD = -FLT_MAX, // 最小のLOD
				   .MaxLOD = +FLT_MAX, // 最大のLOD
		};
		if (FAILED(d3d_device->CreateSamplerState(&desc, &shadow_sampler)))
			return false;

	}
	shadow_infos.resize(cascade_count);
	shadowmap_view_projs.resize(cascade_count);


	is_initialized = true;
	return true;
}

void ShadowMapObject::ShadowMapDrawBegin()
{
	//シャドウマップの描画準備
	auto& camera = GetScene()->GetCurrentCameraRef();
	if (!camera)
		return;
	ClearDepth(shadow_map.raw_shared().get(), 1.0f);

	mat4x4 mat_camera_world = mat4x4(camera->owner.lock()->transform->rotation);
	mat_camera_world[3] = Vector4(camera->owner.lock()->transform->position, 1.0f);

	float camera_near_z = camera->camera_near;
	float camera_far_z = camera->camera_far;

	const float aspect_ratio = ((float)SCREEN_W / (float)SCREEN_H);
	std::vector<float> split_distance{ 20.0f, 100.0f, 500.0f, 1200.0f };
	if (split_distance.size() < cascade_count)
		for (u32 i = split_distance.size(); i < cascade_count; i++)
			split_distance.push_back(min(split_distance[i - 1] + 500, camera_far_z));
	//light_dir.y = sinf(Time::GetTimeFromStart());
	light_dir = { 0,8,-5 };
	light_dir.normalize();
	for (u32 cascade_index = 0; cascade_index < cascade_count; cascade_index++)
	{
		auto& info = shadow_infos[cascade_index];
		info.mat_camera_world_ = mat_camera_world;
		info.fov_y_ = DEG2RAD(camera->perspective);
		info.aspect_ratio_ = aspect_ratio;
		info.near_z_ = (cascade_index == 0) ? camera_near_z : split_distance[cascade_index - 1];
		info.far_z_ = split_distance[cascade_index];

		info.update();

		Vector3 center = info.bounding_sphere_.getXYZ();
		float  radius = info.bounding_sphere_.w;
		float lite_y = 700;

		Vector3 position = center + light_dir * lite_y;
		Vector3 lookat = position - light_dir;
		shadowmap_view = CreateMatrix::lookAtLH(position, lookat, Vector3(0, 1, 0));

		// シャドウマップの上下左右の映る範囲を計算
												//      +---top---+
		float left = -radius;					//      |    |    |
		float right = +radius;					// left +----+----+ right
		float bottom = -radius;					//      |    |    |
		float top = +radius;					//      +--bottom-+

		float near_z = 0.05f * radius;		// シャドウマップの近くのクリッピング面
		float far_z = 2000;		// シャドウマップの遠くのクリッピング面


		shadowmap_proj = CreateMatrix::orthographicOffCenterLH(left, right, bottom, top, near_z, far_z);
		shadowmap_view_projs[cascade_index] = shadowmap_proj * shadowmap_view;

		auto d3d_context = GetD3DDeviceContext();

		u32 x = shadow_map_size * cascade_index;
		D3D11_VIEWPORT vp{};
		vp.TopLeftX = static_cast<FLOAT>(x);
		vp.TopLeftY = 0.0f;
		vp.Width = static_cast<FLOAT>(shadow_map_size);
		vp.Height = static_cast<FLOAT>(shadow_map_size);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		SetRenderTarget(nullptr, shadow_map.raw_shared().get());

		d3d_context->RSSetViewports(1, &vp);



		DxLib::SetCameraViewMatrix(cast(CastPhysXMat(shadowmap_view)));
		DxLib::SetupCamera_ProjectionMatrix(cast(CastPhysXMat(shadowmap_proj)));

		//シャドウマップに対し、全モデルを描画する
		for (auto renderer : model_renderers) {
			if (!renderer)
				continue;
			if (!renderer->CastShadow())
				continue;
			renderer->Draw();
		}



	}



}

void ShadowMapObject::ShadowMapDrawEnd()
{
	auto d3d_context = GetD3DDeviceContext();
	//シャドウマップの描画終了
	SetRenderTarget(nullptr, nullptr);
	auto camera = GetScene()->GetCurrentCameraRef();
	if (camera)
		camera->PrepareCamera();
	d3d_context->PSSetSamplers(13, 1, &shadow_sampler);
	for (auto info : shadow_infos) {
		info.draw();
	}

}

void ShadowInfo::update()
{
	// シャドウマップのフラスタム頂点を計算
	{
		auto& v = frustum_vertices_;

		Vector3 right = mat_camera_world_[0].getXYZ().getNormalized();
		Vector3 up = mat_camera_world_[1].getXYZ().getNormalized();
		Vector3 front = mat_camera_world_[2].getXYZ().getNormalized();
		Vector3 position = mat_camera_world_[3].getXYZ();

		// nearの高さを求める
		//
		//      /|
		//     /-|fovy
		//    /  |     ↑
		//   /   |  near_z_
		//  /    |     ↓
		// +-----+
		//   h0
		//
		// h0 / near_z_ = tan(fov_y_)

		float nz = near_z_;
		float fz = far_z_;

		float h0 = tanf(fov_y_ * 0.5f) * nz;
		float h1 = tanf(fov_y_ * 0.5f) * fz;

		v[0] = position - (right * h0 * aspect_ratio_) + (up * h0) + (front * nz);
		v[1] = position + (right * h0 * aspect_ratio_) + (up * h0) + (front * nz);
		v[2] = position - (right * h0 * aspect_ratio_) - (up * h0) + (front * nz);
		v[3] = position + (right * h0 * aspect_ratio_) - (up * h0) + (front * nz);

		v[4] = position - (right * h1 * aspect_ratio_) + (up * h1) + (front * fz);
		v[5] = position + (right * h1 * aspect_ratio_) + (up * h1) + (front * fz);
		v[6] = position - (right * h1 * aspect_ratio_) - (up * h1) + (front * fz);
		v[7] = position + (right * h1 * aspect_ratio_) - (up * h1) + (front * fz);
	}

	// 8点の中心座標と最大半径を求める
	// 「8点が外接する球」を計算する
	Vector3 center;
	float  radius;
	ShadowMapHelper::ComputeBoundingSphere(frustum_vertices_, 8, center, radius);
	bounding_sphere_ = Vector4(center, radius);
}

void ShadowInfo::draw()
{
	DxLib::SetUseLighting(false);

	// near
	for (u32 i = 0; i < 4; ++i) {
		auto& v = frustum_vertices_[i];
		DxLib::DrawSphere3D(cast(v), 0.1f * near_z_, 32, GetColor(255, 255, 0), 0, 0);
	}
	// far
	for (u32 i = 4; i < 8; ++i) {
		auto& v = frustum_vertices_[i];
		DxLib::DrawSphere3D(cast(v), 0.1f * near_z_, 32, GetColor(255, 0, 255), 0, 0);
	}

	DxLib::SetUseLighting(true);
}
