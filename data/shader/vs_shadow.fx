//----------------------------------------------------------------------------
//!	@file	vs_model.fx
//!	@brief	DxLib MV1モデル頂点シェーダー
//----------------------------------------------------------------------------
#include "dxlib_vs_model.h"

// 頂点シェーダーの出力
struct VS_OUTPUT_MODEL
{
    float4 position_ : SV_Position; //!< 座標       (スクリーン空間)
};
cbuffer ConstBuffer : register(b13)
{
    float4x4 g_LightViewProj;
};
//------------------------------------------------------------s----------------
//	メイン関数
//----------------------------------------------------------------------------
VS_OUTPUT_MODEL main(VS_INPUT_MV1 input)
{
    VS_OUTPUT_MODEL output;

    
	//----------------------------------------------------------
	// 頂点座標変換
	//----------------------------------------------------------
    float3 localPosition = input.position_;
    float3x4 matWorld = DxLib_WorldMatrix(input);

    float3 worldPosition = mul(matWorld, float4(localPosition, 1.0)); // スキニング計算。ワールド空間へ変換
    //float3 viewPosition = mul(DxLib_ViewMatrix(), float4(worldPosition, 1.0)); // ビュー空間へ変換
    //output.position_ = mul(DxLib_ProjectionMatrix(), float4(viewPosition, 1.0)); // スクリーン空間へ変換
    output.position_ = mul(float4(worldPosition, 1.0f), g_LightViewProj);
    return output;
}
