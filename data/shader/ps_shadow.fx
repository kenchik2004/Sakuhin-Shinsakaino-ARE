//----------------------------------------------------------------------------
//!	@file	ps_model.fx
//!	@brief	MV1モデルピクセルシェーダー
//----------------------------------------------------------------------------
#include "dxlib_ps.h"


// 頂点シェーダーの出力
struct VS_OUTPUT_MODEL
{
    float4 position_ : SV_Position; //!< 座標       (スクリーン空間)
};

typedef VS_OUTPUT_MODEL PS_INPUT_MODEL;

//----------------------------------------------------------------------------
// メイン関数
//----------------------------------------------------------------------------
PS_OUTPUT main(PS_INPUT_MODEL input)
{
    PS_OUTPUT output;
    float z = input.position_.z / input.position_.w;
    float depth = z;
    
    output.color0_ = float4(0, 0, 0, 1);
    return output;

}
