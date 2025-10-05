//---------------------------------------------------------------------------
//	@file	Render.cpp
//	@brief	描画関数のDXライブラリ拡張
//---------------------------------------------------------------------------
#include "Render.h"
#include "System/TextureManager.h"
#include "System/Main.h"
#include "System/Shader.h"

namespace {

	std::shared_ptr<Texture>  texture_back_buffer_;      //!< バックバッファ
	std::shared_ptr<Texture>  texture_depth_stencil_;    //!< デフォルトのデプスステンシル
	std::shared_ptr<ShaderPs> shader_ps_texture_;        //!< 通常コピー用のシェーダー

	TargetDesc target_desc_;    //!< 現在のRenderTarget情報

}    // namespace

//---------------------------------------------------------------------------
//! Render初期化
//---------------------------------------------------------------------------
bool RenderInit()
{
	//----------------------------------------------------------
	// バックバッファを取得
	//----------------------------------------------------------
	auto* d3d_back_buffer = static_cast<ID3D11Texture2D*>(const_cast<void*>(GetUseDirect3D11BackBufferTexture2D()));
	auto* d3d_depth_stencil = static_cast<ID3D11Texture2D*>(const_cast<void*>(GetUseDirect3D11DepthStencilTexture2D()));

	texture_back_buffer_ = std::make_shared<Texture>(d3d_back_buffer);
	texture_depth_stencil_ = std::make_shared<Texture>(d3d_depth_stencil);

	//----------------------------------------------------------
	// シェーダー作成
	//----------------------------------------------------------
	shader_ps_texture_ = std::make_shared<ShaderPs>("data/Shader/ps_texture");

	return true;
}

//---------------------------------------------------------------------------
//! Render終了
//---------------------------------------------------------------------------
void RenderExit()
{
	texture_back_buffer_.reset();
	texture_depth_stencil_.reset();
	shader_ps_texture_.reset();
}

//---------------------------------------------------------------------------
//! D3Dデバイスを取得
//---------------------------------------------------------------------------
ID3D11Device* GetD3DDevice()
{
	return reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
}

//---------------------------------------------------------------------------
//! D3Dデバイスコンテキストを取得
//---------------------------------------------------------------------------
ID3D11DeviceContext* GetD3DDeviceContext()
{
	return reinterpret_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));
}

//---------------------------------------------------------------------------
//! 現在のカラーバッファをクリア
//---------------------------------------------------------------------------
void ClearColor(const Color& clear_color)
{
	auto* color_texture = target_desc_.color_targets_[0];

	if (color_texture == nullptr)
		return;

	ClearColor(color_texture, clear_color);
}

//---------------------------------------------------------------------------
//! 現在のデプスバッファをクリア
//---------------------------------------------------------------------------
void ClearDepth(f32 depth_value)
{
	auto* depth_stencil = target_desc_.depth_stencil_;

	if (depth_stencil == nullptr)
		return;

	ClearDepth(depth_stencil, depth_value);
}

//---------------------------------------------------------------------------
//! 現在のステンシルバッファをクリア
//---------------------------------------------------------------------------
void ClearStencil(u8 stencil_value)
{
	auto* depth_stencil = target_desc_.depth_stencil_;

	if (depth_stencil == nullptr)
		return;

	ClearStencil(depth_stencil, stencil_value);
}

//---------------------------------------------------------------------------
//! カラーバッファをクリア
//---------------------------------------------------------------------------
void ClearColor(const Texture* texture, const Color& clear_color)
{
	GetD3DDeviceContext()->ClearRenderTargetView(texture->Rtv(), clear_color.color());
}

//---------------------------------------------------------------------------
//! デプスバッファをクリア
//---------------------------------------------------------------------------
void ClearDepth(const Texture* texture, f32 depth_value)
{
	GetD3DDeviceContext()->ClearDepthStencilView(texture->Dsv(), D3D11_CLEAR_DEPTH, depth_value, 0);
}

//---------------------------------------------------------------------------
//! ステンシルバッファをクリア
//---------------------------------------------------------------------------
void ClearStencil(const Texture* texture, u8 stencil_value)
{
	GetD3DDeviceContext()->ClearDepthStencilView(texture->Dsv(), D3D11_CLEAR_STENCIL, stencil_value, 0);
}

//---------------------------------------------------------------------------
//! 現在のRenderTarget情報を取得
//---------------------------------------------------------------------------
TargetDesc GetRenderTarget()
{
	return target_desc_;
}

//---------------------------------------------------------------------------
//! RenderTargetを一括設定 (TargetDesc指定)
//---------------------------------------------------------------------------
void SetRenderTarget(const TargetDesc& desc)
{
	// 保存
	target_desc_ = desc;

	// カラーターゲットを配列として取得
	std::array<ID3D11RenderTargetView*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> rtvs;
	rtvs.fill(nullptr);
	bool depth_only_pass = true;
	for (u32 i = 0; i < desc.color_count_; ++i) {
		rtvs[i] = desc.color_targets_[i] ? desc.color_targets_[i]->Rtv() : nullptr;
		if (rtvs[i] != nullptr)
			depth_only_pass = false;
	}

	// デプスバッファ
	ID3D11DepthStencilView* dsv = desc.depth_stencil_ ? desc.depth_stencil_->Dsv() : nullptr;

	// [DxLib] DXライブラリ内部のプリミティブバッファをフラッシュさせる
	RenderVertex();

	// RenderTarget設定
	GetD3DDeviceContext()->OMSetRenderTargets(desc.color_count_, depth_only_pass ? nullptr : rtvs.data(), dsv);

	//----------------------------------------------------------
	// ビューポートとシザー領域を更新
	//----------------------------------------------------------
	{
		// RenderTargetの解像度を取得
		u32 w;
		u32 h;
		if(desc.color_targets_[0]){
			// カラーターゲットがあるならそれを優先
			w= desc.color_targets_[0]->Width();
			h= desc.color_targets_[0]->Height();
		}
		else if(desc.depth_stencil_){
			// デプスステンシルがあるならそれを優先
			w= desc.depth_stencil_->Width();
			h= desc.depth_stencil_->Height();
		}
		else{
			// どちらもないならバックバッファを使う
			w= texture_back_buffer_->Width();
			h= texture_back_buffer_->Height();
		}

		D3D11_VIEWPORT viewport{ 0, 0, static_cast<f32>(w), static_cast<f32>(h), 0.0f, 1.0f };
		GetD3DDeviceContext()->RSSetViewports(1, &viewport);

		D3D11_RECT rect{ 0, 0, static_cast<LONG>(w), static_cast<LONG>(h) };
		GetD3DDeviceContext()->RSSetScissorRects(1, &rect);
	}
}

//---------------------------------------------------------------------------
//! RenderTargetを設定
//---------------------------------------------------------------------------
void SetRenderTarget(Texture* color_target, Texture* depth_stencil)
{
	SetRenderTarget(color_target ? 1 : 0, &color_target, depth_stencil);
}

//---------------------------------------------------------------------------
//! RenderTargetを設定(Multiple Render Target)
//---------------------------------------------------------------------------
void SetRenderTarget(u32 color_count, Texture** color_targets, Texture* depth_stencil)
{
	TargetDesc desc;

	desc.color_count_ = color_count;
	// テクスチャ配列をコピー
	for (u32 i = 0; i < color_count; ++i) {
		desc.color_targets_[i] = color_targets[i];
	}
	desc.depth_stencil_ = depth_stencil;

	// 設定
	SetRenderTarget(desc);
}

//---------------------------------------------------------------------------
//! テクスチャを設定
//! @attention 16番以下はDxLibの管理下にあるため設定しないでください。
//---------------------------------------------------------------------------
void SetTexture(u32 slot, Texture* texture)
{
	if (slot <= 16 && slot > 128)
		return;

	ID3D11ShaderResourceView* srv = texture ? texture->Srv() : nullptr;
	GetD3DDeviceContext()->PSSetShaderResources(slot, 1, &srv);
}

//---------------------------------------------------------------------------
//! デフォルトのバックバッファを取得 (R8G8B8A8_UNORM)
//---------------------------------------------------------------------------
Texture* GetBackBuffer()
{
	return texture_back_buffer_.get();
}

//---------------------------------------------------------------------------
//! デフォルトのデプスバッファを取得 (D32_FLOAT)
//---------------------------------------------------------------------------
Texture* GetDepthStencil()
{
	return texture_depth_stencil_.get();
}

//---------------------------------------------------------------------------
//! RenderTargetにイメージをコピー
//---------------------------------------------------------------------------
void CopyToRenderTarget(Texture* dest_render_target, Texture* source_texture, int shader_ps_handle)
{
	// 今のRenderTarget情報を保存しておく(関数終了時に復元するため)
	auto current_target_desc = GetRenderTarget();

	// 描画先を変更
	SetRenderTarget(dest_render_target);

	SetDrawMode(DX_DRAWMODE_BILINEAR);             // テクスチャをバイリニア補間
	SetTextureAddressMode(DX_TEXADDRESS_CLAMP);    // テクスチャを繰り返しなし

	//----------------------------------------------------------
	//! フルスクリーン2D三角形の描画
	//! @see GDC'14 Vertex Shader Tricks
	//! 通常の四角形描画では三角形2枚で描画しますが、斜めのエッジ部分が2回ラスタライズされます。
	//! 無駄を省くために巨大な三角形で覆うことでこの問題を解決できます。(数%の高速化効果)
	//  A
	//  |  ＼            本来のスクリーンを△ABCで覆うと有効ピクセル部分は
	//  |      ＼        矩形ラスタライズされるようになります
	//  |          ＼
	//  +------+------+
	//  |      |      |  ＼
	//  +------+------+      ＼
	//  |      |      |          ＼
	//  B-------------+-------------C
	//----------------------------------------------------------
	{
		f32 w = static_cast<f32>(SCREEN_W);    // 解像度 幅
		f32 h = static_cast<f32>(SCREEN_H);    // 解像度 高さ

		VERTEX2DSHADER v[3]{};

		// 頂点
		v[0].pos = { 0.0f, -h, 0.0f };                  // 2D座標
		v[0].rhw = 1.0f;                              // rhw = 1.0f 初期化は2D描画に必須
		v[0].dif = GetColorU8(255, 255, 255, 255);    // カラー
		v[0].u = 0.0f;                              // テクスチャ座標 U
		v[0].v = -1.0f;                             // テクスチャ座標 V

		v[1].pos = { 0.0f, h, 0.0f };                   // 2D座標
		v[1].rhw = 1.0f;                              // rhw = 1.0f 初期化は2D描画に必須
		v[1].dif = GetColorU8(255, 255, 255, 255);    // カラー
		v[1].u = 0.0f;                              // テクスチャ座標 U
		v[1].v = 1.0f;                              // テクスチャ座標 V

		v[2].pos = { w * 2.0f, h, 0.0f };               // 2D座標
		v[2].rhw = 1.0f;                              // rhw = 1.0f 初期化は2D描画に必須
		v[2].dif = GetColorU8(255, 255, 255, 255);    // カラー
		v[2].u = 2.0f;                              // テクスチャ座標 U
		v[2].v = 1.0f;                              // テクスチャ座標 V

		// 使用するテクスチャを設定 (slot=0)
		SetUseTextureToShader(0, *source_texture);

		// 使用するピクセルシェーダーを設定 (2Dの場合は頂点シェーダー不要)
		int ps = (shader_ps_handle != -1) ? shader_ps_handle : *shader_ps_texture_;
		SetUsePixelShader(ps);

		// 描画
		DrawPrimitive2DToShader(v, 3, DX_PRIMTYPE_TRIANGLELIST);
	}
	// 使い終わったらテクスチャ設定を解除
	// 解除しておかないとモデル描画に影響あり。
	SetUseTextureToShader(0, -1);
	SetUsePixelShader(-1);

	// 描画先を元に戻す
	SetRenderTarget(current_target_desc);    // 元に戻す
}
