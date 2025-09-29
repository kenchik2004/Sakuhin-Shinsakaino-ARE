//----------------------------------------------------------------------------
//!	@file	ps_model.fx
//!	@brief	MV1モデルピクセルシェーダー
//----------------------------------------------------------------------------
#include "dxlib_ps.h"


// 頂点シェーダーの出力
struct VS_OUTPUT_MODEL
{
    float4 position_ : SV_Position; //!< 座標       (スクリーン空間)
    float4 curr_position_ : CURR_POSITION; //!< 現在の座標 (スクリーン空間)
    float3 world_position_ : WORLD_POSITION; //!< ワールド座標
    float3 normal_ : NORMAL0; //!< 法線
    float4 diffuse_ : COLOR0; //!< Diffuseカラー
    float2 uv0_ : TEXCOORD0; //!< テクスチャ座標
    float4 prev_position_ : PREV_POSITION; //!< 1フレーム前の座標 (スクリーン空間) ※末尾に追加されているため注意
};

typedef VS_OUTPUT_MODEL PS_INPUT_MODEL;




static const float PI = 3.14159265359;

//----------------------------------------------------------------------------
// メイン関数
//----------------------------------------------------------------------------
PS_OUTPUT main(PS_INPUT_MODEL input)
{
    PS_OUTPUT output;

    float2 uv = input.uv0_;
    float3 N = normalize(input.normal_); // 法線
	
	//------------------------------------------------------------
	// 法線マップ
	//------------------------------------------------------------
    N = Normalmap(N, input.world_position_, uv);

	//------------------------------------------------------------
	// テクスチャカラーを読み込み
	//------------------------------------------------------------
    float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, uv);

    // アルファテスト
    if (textureColor.a < 0.5)
        discard;

    output.color0_ = input.diffuse_*textureColor;
    output.color0_.rgb;
	
	
	
	// 出力パラメータを返す
    return output;
}
