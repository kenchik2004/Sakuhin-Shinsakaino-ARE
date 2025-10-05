//---------------------------------------------------------------------------
//	@file	Render.h
//	@brief	描画関数のDXライブラリ拡張
//---------------------------------------------------------------------------
#pragma once

class Texture;

//===========================================================================
//!	@name	DirectX11オブジェクト
//===========================================================================
//@{

//! D3Dデバイスを取得
ID3D11Device* GetD3DDevice();

//! D3Dデバイスコンテキストを取得
ID3D11DeviceContext* GetD3DDeviceContext();

//@}
//===========================================================================
//!	@name	クリア関数のDXライブラリ拡張
//===========================================================================
//@{

//! 現在のカラーバッファをクリア
//!	@param	[in]	clear_color  	クリアするカラー値
void ClearColor(const Color& clear_color);

//! 現在のデプスバッファをクリア
//!	@param	[in]	depth_value  	クリアするデプス値
void ClearDepth(f32 depth_value);

//! 現在のステンシルバッファをクリア
//!	@param	[in]	stencil_value	クリアするステンシル値
void ClearStencil(u8 stencil_value);

//! カラーバッファをクリア
//!	@param	[in]	texture	    	対象のカラーバッファ
//!	@param	[in]	clear_color  	クリアするカラー値
void ClearColor(const Texture* texture, const Color& clear_color);

//! デプスバッファをクリア
//!	@param	[in]	texture 		対象のデプスバッファ
//!	@param	[in]	depth_value	    クリアするデプス値
void ClearDepth(const Texture* texture, f32 depth_value);

//! ステンシルバッファをクリア
//!	@param	[in]	texture		    対象のデプスバッファ
//!	@param	[in]	stencil_value	クリアするステンシル値
void ClearStencil(const Texture* texture, u8 stencil_value);

//@}
//===========================================================================
//!	@name	RenderTarget設定のDXライブラリ拡張
//===========================================================================
//@{

//! RenderTarget情報
struct TargetDesc {
    u32                                                          color_count_   = 0;
    std::array<Texture*, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> color_targets_ = {};
    Texture*                                                     depth_stencil_ = nullptr;
};

//! 現在のRenderTarget情報を取得
TargetDesc GetRenderTarget();

//! RenderTargetを一括設定 (TargetDesc指定)
//!	@param	[in]    desc	RenderTarget情報
void SetRenderTarget(const TargetDesc& desc);

//! RenderTargetを設定 (単体)
//!	@param	[in]	color_target	カラーターゲットの先頭アドレス
//!	@param	[in]	depth_stencil	デプスバッファ(nullptr指定で無効化)
void SetRenderTarget(Texture* color_target, Texture* depth_stencil = nullptr);

//! RenderTargetを設定 (Multiple Render Target)
//!	@param	[in]	color_count  	カラーターゲット数(1～8)
//!	@param	[in]	color_targets	カラーターゲット配列の先頭アドレス
//!	@param	[in]	depth_stencil	デプスバッファ(nullptr指定で無効化)
void SetRenderTarget(u32 color_count, Texture** color_targets, Texture* depth_stencil = nullptr);

//! テクスチャを設定
//!	@param	[in]	slot	テクスチャSlot番号(t16～)
//!	@param	[in]	texture 設定するテクスチャ(nullptrの場合は解除)
void SetTexture(u32 slot, Texture* texture);

//! デフォルトのバックバッファを取得(R8G8B8A8_UNORM)
Texture* GetBackBuffer();

//! デフォルトのデプスバッファを取得(D32_FLOAT)
Texture* GetDepthStencil();

//@}

//! RenderTargetにイメージをコピー
//!	@param	[out]	dst_render_target	出力先RenderTarget
//! @param	[in]	src_texture		    コピー元テクスチャ
//! @param	[in]	shader_ps_handle  	カスタムのピクセルシェーダー (-1の場合は単純コピー)
void CopyToRenderTarget(Texture* dst_render_target, Texture* src_texture, int shader_ps_handle = -1);

//===========================================================================
//!	@name	システム関数
//===========================================================================
//@{

//! Render初期化
bool RenderInit();

//! Render終了
void RenderExit();

//@}
