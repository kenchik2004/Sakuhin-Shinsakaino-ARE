//----------------------------------------------------------------------------
//!	@file	ps_model_velocity.fx
//!	@brief	速度バッファを生成 ピクセルシェーダー
//----------------------------------------------------------------------------
#include "dxlib_ps.h"

// 頂点シェーダーの出力
struct VS_OUTPUT_MODEL
{
	float4	position_       : SV_Position;      //!< 座標       (スクリーン空間)
    float4  curr_position_  : CURR_POSITION;    //!< 現在の座標 (スクリーン空間)
	float3	world_position_ : WORLD_POSITION;   //!< ワールド座標
	float3	normal_         : NORMAL0;          //!< 法線
	float4	diffuse_        : COLOR0;           //!< Diffuseカラー
	float2	uv0_            : TEXCOORD0;        //!< テクスチャ座標
    float4  prev_position_  : PREV_POSITION;    //!< 1フレーム前の座標 (スクリーン空間) ※末尾に追加されているため注意
};

typedef	VS_OUTPUT_MODEL	PS_INPUT_MODEL;

//----------------------------------------------------------------------------
// メイン関数
//----------------------------------------------------------------------------
PS_OUTPUT main(PS_INPUT_MODEL input)
{
	PS_OUTPUT	output;

	//----------------------------------------------------------
	// スクリーン座標を計算するためにWで除算
	// -1～+1 のスクリーン空間の座標を求める
	//----------------------------------------------------------
	float2	curr_position = input.curr_position_.xy / input.curr_position_.w;
	float2	prev_position = input.prev_position_.xy / input.prev_position_.w;

	// UV空間の移動量を計算
	float2	velocity = (curr_position - prev_position) * float2(0.5, -0.5);

	// カラーで縦横の速度成分に着色して出力
	output.color0_.rg = velocity * 100;
	output.color0_.b  = 0.0;
	output.color0_.a  = 1.0;

	// 出力パラメータを返す
	return output;
}
