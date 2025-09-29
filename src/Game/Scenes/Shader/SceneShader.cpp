
#include "precompile.h"
#include "SceneShader.h"
#include "System/Components/Camera.h"

Vector3 light_pos = { 0, 30, -100 };
Vector3 light_look = { 0, 0, 0 };
Vector3 light_up = { 0, 1, 0 };
constexpr float sh_size = 1024;
const Vector2 shadowmap_size = { sh_size, sh_size }; //!< シャドウマップのサイズ
Vector2 shadowmap_xy = { 10,10 };
int const_buffer = -1;
int cam_const_buffer = -1;
int shadow_const_buffer = -1;
Vector2 near_far = {};
Vector3 frustum_vertices[8];
struct CameraInfo
{
	mat4x4 mat_view_; //!< ビュー行列
	mat4x4 mat_proj_; //!< 投影行列
	Vector3 eye_position_; //!< カメラの位置
	float a;
};

struct LightInfo
{
	mat4x4  mat_view_proj_; //!< ビュー投影行列
	Vector4 light_color_; //!< 光源の色
	Vector3 light_direction_; //!< 光源の方向
	float a;
};
struct ShadowInfo {
	Vector3 frustum_vertices[8];
};


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
		{axis_x.x, axis_y.x, axis_z.x, 0.0f},
		{axis_x.y, axis_y.y, axis_z.y, 0.0f},
		{axis_x.z, axis_y.z, axis_z.z, 0.0f},
		{      tx,       ty,       tz, 1.0f},
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
		{           rcp_width * 2.0f,                         0.0f,            0.0f, 0.0f},
		{                       0.0f,            rcp_height * 2.0f,            0.0f, 0.0f},
		{                       0.0f,                         0.0f,           range, 0.0f},
		{-(left + right) * rcp_width, -(top + bottom) * rcp_height, -range * near_z, 1.0f}
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
		{width,   0.0f,            0.0f, 0.0f},
		{ 0.0f, height,            0.0f, 0.0f},
		{ 0.0f,   0.0f,           range, 1.0f},
		{ 0.0f,   0.0f, -range * near_z, 0.0f}
	};
	return mat4x4(m[0], m[1], m[2], m[3]);
}

int SceneShader::Init()
{
#if 1
	const_buffer = CreateShaderConstantBuffer(sizeof(LightInfo));
	shadow_const_buffer = CreateShaderConstantBuffer(sizeof(LightInfo));
	cam_const_buffer = CreateShaderConstantBuffer(sizeof(CameraInfo));
	ID3D11Device* d3d_device = reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
	D3D11_SAMPLER_DESC desc
	{
	D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
	D3D11_TEXTURE_ADDRESS_CLAMP,
	D3D11_TEXTURE_ADDRESS_CLAMP,
	D3D11_TEXTURE_ADDRESS_CLAMP,
	0.0f,
	1,
	D3D11_COMPARISON_LESS,
		{1.0f,1.0f,1.0f,1.0f},
	-FLT_MAX,
	FLT_MAX

	};
	d3d_device->CreateSamplerState(&desc, &d3d_shadow_sampler);


	model[0] = MV1LoadModel("data/Player.mv1");
	model[1] = MV1LoadModel("data/Stage/SwordBout/Stage00.mv1");
	auto variant = 8;
	shader_vs = std::make_shared<ShaderVs>("data/shader/vs_model.fx", variant);
	shader_ps = std::make_shared<ShaderPs>("data/shader/ps_model.fx");
	light_shader_vs = std::make_shared<ShaderVs>("data/shader/vs_shadow.fx", variant);
	light_shader_ps = std::make_shared<ShaderPs>("data/shader/ps_shadow.fx");
	SetDrawValidGraphCreateZBufferFlag(true);
	SetCreateDrawValidGraphZBufferBitDepth(32);
	shadow_map = TextureManager::Create("shadow_map", static_cast<int>(shadowmap_size.x), static_cast<int>(shadowmap_size.y));
	model_obj = SceneManager::Object::Create<GameObject>();
	model_obj->transform->position.y = 3;
	MV1SetPosition(model[0], VGet(0, 1.15f, 0));
	MV1SetPosition(model[1], VGet(0, -45, 0));
	MV1SetScale(model[0], VGet(0.01f, 0.01f, 0.01f));
#endif
	obj = SceneManager::Object::Create<Object>();
	obj->transform->position = { 0,5,-5 };
	obj->transform->AddRotation({ 45,0,0 });
	camera = obj->AddComponent<Camera>();
	camera->camera_far = 5000;

	MV1SetUseZBuffer(model[0], true);
	MV1SetUseZBuffer(model[1], true);
	return 0;
}

void SceneShader::Update()
{

	if (Input::GetKey(KeyCode::F)) {
		//TextureManager::Load("data/title.jpg", "title");
		SafeSharedPtr<Texture> tex;
		//tex= TextureManager::CloneByName("shadow_map");
		tex = shadow_map;
		MV1SetTextureGraphHandle(model[0], 6, tex->GetHandle(), true);
		MV1SetTextureGraphHandle(model[0], 0, tex->GetHandle(), true);
		MV1SetTextureGraphHandle(model[0], 3, tex->GetHandle(), true);

	}
	//light_pos = Quaternion(DEG2RAD(Time::DeltaTime() * 10), Vector3(0, 1, 0).getNormalized()).rotate(light_pos);

	if (Input::GetKey(KeyCode::Up))
		obj->transform->AddRotation(Quaternion(DEG2RAD(Time::DeltaTime() * -45), { 1,0,0 }));
	if (Input::GetKey(KeyCode::Down))
		obj->transform->AddRotation(Quaternion(DEG2RAD(Time::DeltaTime() * 45), { 1,0,0 }));
	//	if (Input::GetKeyDown(KeyCode::Return))
		//	obj->transform->AddRotation(Quaternion(DEG2RAD(180), { 0,1,0 }));

	if (Input::GetKey(KeyCode::Left))
		obj->transform->AddRotation(Quaternion(DEG2RAD(Time::DeltaTime() * -45), { 0,1,0 }));
	if (Input::GetKey(KeyCode::Right))
		obj->transform->AddRotation(Quaternion(DEG2RAD(Time::DeltaTime() * 45), { 0,1,0 }));
	if (Input::GetKey(KeyCode::Space))
		obj->transform->position += Time::DeltaTime() * obj->transform->AxisY() * 20;
	if (Input::GetKey(KeyCode::LControl))
		obj->transform->position += Time::DeltaTime() * obj->transform->AxisY() * -20;
	if (Input::GetKey(KeyCode::W))
		obj->transform->position += Time::DeltaTime() * obj->transform->AxisZ() * 20;
	if (Input::GetKey(KeyCode::S))
		obj->transform->position += Time::DeltaTime() * -obj->transform->AxisZ() * 20;
	if (Input::GetKey(KeyCode::D))
		obj->transform->position += Time::DeltaTime() * obj->transform->AxisX() * 20;
	if (Input::GetKey(KeyCode::A))
		obj->transform->position += Time::DeltaTime() * obj->transform->AxisX() * -20;
	{
		if (Input::GetKey(KeyCode::I))
			model_obj->transform->position += { 0, 0, Time::DeltaTime() * 5 };
		if (Input::GetKey(KeyCode::K))
			model_obj->transform->position -= {0, 0, Time::DeltaTime() * 5};
		if (Input::GetKey(KeyCode::L))
			model_obj->transform->position += {Time::DeltaTime() * 5, 0, 0};
		if (Input::GetKey(KeyCode::J))
			model_obj->transform->position -= {Time::DeltaTime() * 5, 0, 0 };
		if (Input::GetKey(KeyCode::O))
			model_obj->transform->position += {0, Time::DeltaTime() * 5, 0};
		if (Input::GetKey(KeyCode::P))
			model_obj->transform->position -= {0, Time::DeltaTime() * 5, 0 };
	}

	DxLib::MV1SetRotationXYZ(model[0], VGet(0, Time::GetRealTimeFromStart(), 0));
}

void SceneShader::PreDraw()
{
#ifndef  NDEBUG
	//シェーダーのファイル更新を監視(Release版は最初にコンパイルするのでいらない)
	Shader::updateFileWatcher();
#endif // ! _DEBUG
}

void SceneShader::Draw()
{

#if 1
	SetUseZBuffer3D(TRUE);
	SetWriteZBuffer3D(TRUE);
	//DxLib::SetDrawScreen(shadow_map->GetHandle());
	auto d3ddevice = reinterpret_cast<ID3D11Device*>(const_cast<void*>(DxLib::GetUseDirect3D11Device()));
	auto d3ddevicecontext = reinterpret_cast<ID3D11DeviceContext*>(const_cast<void*>(DxLib::GetUseDirect3D11DeviceContext()));
	auto shadow_dsv = reinterpret_cast<ID3D11DepthStencilView*>(const_cast<void*>(DxLib::GetGraphID3D11DepthStencilView(shadow_map->GetHandle())));
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)shadowmap_size.x;
	vp.Height = (FLOAT)shadowmap_size.y;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	d3ddevicecontext->RSSetViewports(1, &vp);


	ID3D11RenderTargetView* p_null = nullptr;
	d3ddevicecontext->OMSetRenderTargets(1, &p_null, shadow_dsv);
	d3ddevicecontext->ClearDepthStencilView(shadow_dsv, D3D11_CLEAR_DEPTH, 1.0, 0);
	MV1SetPosition(model[0], cast(model_obj->transform->position));

	//SetCameraNearFar(0.1f, 500.0f);
	//SetCameraPositionAndTargetAndUpVec(cast(light_pos), cast(light_look), cast(light_up));

	//SetupCamera_Perspective(45);
	//SetupCamera_Ortho(20);
	//camera->PreDraw();
	//SetCameraPositionAndTargetAndUpVec(cast(obj->transform->position), cast(obj->transform->position + obj->transform->AxisZ()), cast(obj->transform->AxisY()));
	//ユーザーカスタムシェーダーをセット
	//頂点シェーダー
	DxLib::SetUseVertexShader(*light_shader_vs);
	//ピクセルシェーダー
	DxLib::SetUsePixelShader(*light_shader_ps);
	auto lightProjMat = orthographicOffCenterLH(-shadowmap_xy.x * 0.5f, shadowmap_xy.x * 0.5f, -shadowmap_xy.y * 0.5f, shadowmap_xy.y * 0.5f, 0.1f, 500);
	//auto lightProjMat = lookAtLH(obj->transform->position, obj->transform->position + obj->transform->AxisZ(), obj->transform->AxisY());
	auto lightViewMat = lookAtLH(light_pos + model_obj->transform->position, light_look + model_obj->transform->position, light_up);
	auto lightViewProjMat = lightProjMat * lightViewMat;
	if constexpr (true)
	{
		void* ptr = GetBufferShaderConstantBuffer(shadow_const_buffer);
		auto pptr = reinterpret_cast<LightInfo*>(ptr); {
			pptr->mat_view_proj_ = lightViewProjMat;
			pptr->light_color_ = { 1,1,1,10 };
			pptr->light_direction_ = (light_look - light_pos).getNormalized();
		}
		DxLib::SetShaderConstantBuffer(shadow_const_buffer, DX_SHADERTYPE_VERTEX, 13);
		DxLib::UpdateShaderConstantBuffer(shadow_const_buffer);
	}
	for (int i = 0; i < 2; i++)
		//フレーム(リグ)単位でメッシュを描画
		for (s32 frame_index = 0; frame_index < MV1GetFrameNum(model[i]); frame_index++)
		{
			//フレームのメッシュ数を取得
			s32 mesh_count = MV1GetFrameMeshNum(model[i], frame_index);
			for (s32 mesh_index = 0; mesh_index < mesh_count; mesh_index++)
			{

				//メッシュ番号を取得
				s32 mesh = MV1GetFrameMesh(model[i], frame_index, mesh_index);

				//メッシュのトライアングルリスト数を取得
				s32 tlist_cout = MV1GetMeshTListNum(model[i], mesh);
				for (s32 tlist_index = 0; tlist_index < tlist_cout; tlist_index++)
				{
					//トライアングルリスト番号を取得
					s32 tlist = MV1GetMeshTList(model[i], mesh, tlist_index);

					//トライアングルリストが使用しているマテリアルのインデックスを取得
					auto material_index = MV1GetTriangleListUseMaterial(model[i], tlist);
					//--------------------------------------------------
					// シェーダーバリエーションを選択
					//--------------------------------------------------
					// 頂点データタイプ(DX_MV1_VERTEX_TYPE_1FRAME 等)
					auto vertex_type = MV1GetTriangleListVertexType(model[i], tlist);
					u32  variant_vs = vertex_type;    // DXライブラリの頂点タイプをそのままバリエーション番号に

					//--------------------------------------------------
					// トライアングルリストを描画
					//--------------------------------------------------
					int handle_vs = light_shader_vs->variant(variant_vs);
					int handle_ps = *light_shader_ps;

					// シェーダーがない場合はオリジナルシェーダー利用を無効化
					bool shader_enable = (handle_vs != -1) && (handle_ps != -1);
					DxLib::MV1SetUseOrigShader(shader_enable);
					DxLib::SetUseVertexShader(handle_vs);
					DxLib::SetUsePixelShader(handle_ps);


					//トライアングルリストを描画
					DxLib::MV1DrawTriangleList(model[i], tlist);
				}

				//メッシュごとに描画
				//MV1DrawMesh(model[i], mesh);
			}
		}

	//オリジナルシェーダーを使用するフラグをoff(後片付け)
	DxLib::MV1SetUseOrigShader(false);


	if constexpr (true)
	{
		void* ptr = GetBufferShaderConstantBuffer(const_buffer);
		auto pptr = reinterpret_cast<LightInfo*>(ptr); {
			pptr->mat_view_proj_ = lightViewProjMat;
			pptr->light_direction_ = (light_pos - light_look).getNormalized();
			pptr->light_color_ = { 1,1,1,10 };
		}
		DxLib::SetShaderConstantBuffer(const_buffer, DX_SHADERTYPE_PIXEL, 13);
		DxLib::UpdateShaderConstantBuffer(const_buffer);
	}

	vp.Width = (FLOAT)(SCREEN_W - 1);
	vp.Height = (FLOAT)(SCREEN_H - 1);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	d3ddevicecontext->RSSetViewports(1, &vp);

	DxLib::SetDrawScreen(DX_SCREEN_BACK);
	camera->PreDraw();
	if constexpr (true)
	{
		void* ptr = GetBufferShaderConstantBuffer(cam_const_buffer);
		auto pptr = reinterpret_cast<CameraInfo*>(ptr); {
			pptr->eye_position_ = obj->transform->position;
			pptr->mat_view_ = lookAtLH(obj->transform->position, obj->transform->position + obj->transform->AxisZ(), obj->transform->AxisY());
			pptr->mat_proj_ = perspectiveFovLH(DEG2RAD(45.0f), (float)SCREEN_W / (float)SCREEN_H, 0.1f, 1000.0f);

		}
		DxLib::SetShaderConstantBuffer(cam_const_buffer, DX_SHADERTYPE_PIXEL, 11);
		DxLib::UpdateShaderConstantBuffer(cam_const_buffer);
	}

	for (int i = -10; i <= 10; i++)
		for (int j = -10; j <= 10; j++) {
			DxLib::DrawLine3D(VGet(i, 0, -10), VGet(i, 0, 10), Color::RED);
			DxLib::DrawLine3D(VGet(-10, 0, j), VGet(10, 0, j), Color::RED);
		}

	d3ddevicecontext->CSSetSamplers(13, 1, &d3d_shadow_sampler);

	ID3D11ShaderResourceView* shadow_srv = nullptr;

	ID3D11Resource* shadow_resource = nullptr;
	shadow_dsv->GetResource(&shadow_resource); // テクスチャ本体

	ID3D11Texture2D* tex = nullptr;
	shadow_resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&tex);
	shadow_resource->Release(); // QIしたのでこちらはRelease

	D3D11_TEXTURE2D_DESC texDesc;
	tex->GetDesc(&texDesc);

	// typelessであることを確認（RenderDoc情報通りならOK）
	if (texDesc.Format == DXGI_FORMAT_R32_TYPELESS) {

		D3D11_SHADER_RESOURCE_VIEW_DESC shadow_desc = {};
		shadow_desc.Format = DXGI_FORMAT_R32_FLOAT; // シェーダー用フォーマットに変換
		shadow_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		shadow_desc.Texture2D.MipLevels = texDesc.MipLevels;
		shadow_desc.Texture2D.MostDetailedMip = 0;

		ID3D11ShaderResourceView* depthSRV = nullptr;
		HRESULT hr = d3ddevice->CreateShaderResourceView(tex, &shadow_desc, &depthSRV);
		if (!FAILED(hr)) {



			d3ddevice->CreateShaderResourceView(tex, &shadow_desc, &shadow_srv);
			d3ddevicecontext->PSSetShaderResources(8, 1, &shadow_srv); // テクスチャ
		}

		if (depthSRV) depthSRV->Release();
	}
	if (tex)tex->Release();
	if (shadow_srv)shadow_srv->Release();
	//SetUseTextureToShader(8, shadow_map->GetHandle()); // スロット8に渡す
	for (int i = 0; i < 2; i++)
		//フレーム(リグ)単位でメッシュを描画
		for (s32 frame_index = 0; frame_index < MV1GetFrameNum(model[i]); frame_index++)
		{
			//フレームのメッシュ数を取得
			s32 mesh_count = MV1GetFrameMeshNum(model[i], frame_index);
			for (s32 mesh_index = 0; mesh_index < mesh_count; mesh_index++)
			{

				//メッシュ番号を取得
				s32 mesh = MV1GetFrameMesh(model[i], frame_index, mesh_index);

				//メッシュのトライアングルリスト数を取得
				s32 tlist_cout = MV1GetMeshTListNum(model[i], mesh);
				for (s32 tlist_index = 0; tlist_index < tlist_cout; tlist_index++)
				{
					//トライアングルリスト番号を取得
					s32 tlist = MV1GetMeshTList(model[i], mesh, tlist_index);

					//トライアングルリストが使用しているマテリアルのインデックスを取得
					auto material_index = MV1GetTriangleListUseMaterial(model[i], tlist);
					//--------------------------------------------------
					// シェーダーバリエーションを選択
					//--------------------------------------------------
					// 頂点データタイプ(DX_MV1_VERTEX_TYPE_1FRAME 等)
					auto vertex_type = MV1GetTriangleListVertexType(model[i], tlist);
					u32  variant_vs = vertex_type;    // DXライブラリの頂点タイプをそのままバリエーション番号に

					//--------------------------------------------------
					// トライアングルリストを描画
					//--------------------------------------------------
					int handle_vs = shader_vs->variant(variant_vs);
					int handle_ps = *shader_ps;

					// シェーダーがない場合はオリジナルシェーダー利用を無効化
					bool shader_enable = (handle_vs != -1) && (handle_ps != -1);

					DxLib::MV1SetUseOrigShader(shader_enable);
					DxLib::SetUseVertexShader(handle_vs);
					DxLib::SetUsePixelShader(handle_ps);

					//トライアングルリストを描画
					DxLib::MV1DrawTriangleList(model[i], tlist);
				}

				//メッシュごとに描画
				//MV1DrawMesh(model[i], mesh);
			}
		}
	//オリジナルシェーダーを使用するフラグをoff(後片付け)
	DxLib::MV1SetUseOrigShader(false);
	//DxLib::DrawExtendGraph(0, 0, 500, 500, shadow_map, false);
	DxLib::DrawLine3D({ 0,2,0 }, cast(light_pos.getNormalized() * 2 + Vector3(0, 2, 0)), Color::GREEN);
#endif
	DxLib::DrawFormatString(0, 0, Color::RED, "FPS:%.1f", Time::GetFPS());



}

void SceneShader::Exit()
{
	d3d_shadow_sampler->Release();
	shader_ps.reset();
	shader_vs.reset();
	DeleteShaderConstantBuffer(const_buffer);
	DeleteShaderConstantBuffer(shadow_const_buffer);
	DeleteShaderConstantBuffer(cam_const_buffer);
	for (int i = 0; i < 2; i++)
		MV1DeleteModel(model[i]);
	shadow_map.reset();
	return;
}
