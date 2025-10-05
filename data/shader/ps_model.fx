//----------------------------------------------------------------------------
//!	@file	ps_model.fx
//!	@brief	MV1モデルピクセルシェーダー
//----------------------------------------------------------------------------
#include "dxlib_ps.h"

// 定数バッファは b0～b14 まで利用可能

static const float PI = 3.14159265359;


cbuffer CameraInfo : register(b11)
{
    matrix mat_view_; //!< ビュー行列
    matrix mat_proj_; //!< 投影行列
    float3 eye_position_; //!< カメラの位置
    matrix light_view_proj_[4]; //!< ライトのビュー投影行列
};

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


Texture2D ShadowmapTexture : register(t13);
// シャドウデプス比較専用サンプラー
SamplerComparisonState ShadowmapSampler : register(s13);

//==============================================================
// IBL
//==============================================================
TextureCube IBL_Diffuse : register(t14);
TextureCube IBL_Specular : register(t15);

uint Xorshift(uint seed)
{
    uint x = seed;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

// 0.0～1.0の範囲でランダムな値を生成
float rand(float2 co)
{
    return frac(sin(dot(co, float2(12.9898f, 78.233f))) * 43758.5453f);
}

// 0.0～1.0の範囲でランダムな値を生成
float InterleavedGradientNoise(float2 position_screen)
{
    float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(position_screen, magic.xy)));
}


// GGX スペキュラーモデル (Trowbridge Reitz)
// エネルギー保存則(Normalized distribution function) / 微細表面(microfacet)
//!	@param	[in]	N			法線(Normal)
//!	@param	[in]	H			ハーフベクトル(Half)
//!	@param	[in]	roughness	表面の粗さ 0.0(つるつる)～1.0(ざらざら)
//!	@param	[in]	gamma		スペキュラーの減衰テールの係数
float D_GGX(float3 N, float3 H, float roughness)
{
	//                            a^2
	//	D(N, H, a) = -------------------------------------
	//                π * [ (a^2 - 1)( dot(N, H)^2 + 1]^2
	// ※a は roughness の2乗

    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;
    float numerator = a2;
    float denominator = (NdotH2 * (a2 - 1.0) + 1.0);
    denominator = PI * denominator * denominator;
    return numerator / denominator;
}

//!	フレネル反射 (Schlick近似)
//!	[in]	f0		初期の反射率(fresnel bias)
//!	[in]	NdotV	NとVの内積値
float3 F_Schlick(float3 f0, float NdotV)
{
    float x = 1.0 - NdotV;
    return f0 + (1.0 - f0) * (x * x * x * x * x);
}

//! G項 Smith 自己遮蔽モデル
float G_Smith_Schlick_GGX(float roughness, float NdotV, float NdotL)
{
    float a = roughness * roughness;
    float k = a * 0.5f;
    float GV = NdotV / (NdotV * (1 - k) + k);
    float GL = NdotL / (NdotL * (1 - k) + k);

    return GV * GL;
}

//---------------------------------------------------------------------------
// Vogel disk sampling
//---------------------------------------------------------------------------
float2 VogelDiskSample(int index, int numSamples, float angleBias = 0.0f)
{
	// Vogel disk sampling
	// https://www.shadertoy.com/view/4l3yRM
	
	// 
    const float GOLDEN_ANGLE = 2.399963229728653; // 黄金角 (黄金比に基づく角度)
	
    float theta = GOLDEN_ANGLE * float(index) + angleBias;
#if 0
	// リファレンス実装
    float r = sqrt(float(index) + 0.5) / sqrt(float(numSamples));
    return float2(r * cos(theta), r * sin(theta));
#else
	// 最適化版
    float r = sqrt((float(index) + 0.5) / float(numSamples));
    return sin(float2(theta + (PI / 2.0), theta)) * r;
#endif
}


//----------------------------------------------------------------------------
// メイン関数
//----------------------------------------------------------------------------
PS_OUTPUT main(PS_INPUT_MODEL input)
{
    PS_OUTPUT output;

    float2 uv = input.uv0_;
    float3 N = normalize(input.normal_); // 法線
    float3 V = normalize(eye_position_ - input.world_position_);
	
	
    float3 light = float3(0, 0, 5); // ライトのポジション
	
    float3 L = -normalize(light);
	
	//------------------------------------------------------------
	// シャドウ
	//------------------------------------------------------------
	// カスケード番号を選択する
    int cascadeIndex = 0;
	{
        const float split_distance[4] =
        {
            20.0f, 100.0f, 500.0f, 1200.0f
        };
		
        float distance = input.position_.w; // カメラからの距離(m)
  
        if (split_distance[0] < distance)
            cascadeIndex++;
        if (split_distance[1] < distance)
            cascadeIndex++;
        if (split_distance[2] < distance)
            cascadeIndex++;
    }


	
    float shadow = 1.0f; // シャドウの初期値 (1.0:影なし, 0.0:影あり)
	{
        float4 screenPosition = mul(float4(input.world_position_, 1.0f), light_view_proj_[cascadeIndex]);
        screenPosition.xyz /= screenPosition.w; // 正規化

        float2 shadowUv = screenPosition.xy * float2(0.5f, -0.5f) + 0.5f; // [-1, 1] → [0, 1] の変換
 
		
        shadowUv.x = shadowUv.x * 0.25f + 0.25f * cascadeIndex;
		
		
        shadow = 0.0f;

        float thetaBias = InterleavedGradientNoise(input.position_.xy) * (2.0f * PI);
        //float thetaBias = rand(input.position_.xy) * 2.0f * PI;
		
        const int SAMPLE_NUM = 16; // サンプリング数 
        for (int i = 0; i < SAMPLE_NUM; ++i)
        {
            float2 offset = VogelDiskSample(i, SAMPLE_NUM, thetaBias) * (1.0 / 512.0);
		
				// シャドウマップの深度値と比較
            static const float shadowBias = 0.0007f; // シャドウバイアス (深度値のオフセット)

            shadow += ShadowmapTexture.SampleCmp(ShadowmapSampler, shadowUv + offset * float2(0.25f, 1.0f), screenPosition.z - shadowBias).r;
        }
		
        shadow *= 1.0f / SAMPLE_NUM;
 
		//if (shadowDepth + shadowBias < screenPosition.z)
        //{
        //    shadow = 0.0f; // 影あり
        //}
		
		// Lambertと合成
        shadow = min(shadow, saturate(dot(N, L)));
		
		
		// 影を薄くする
        shadow = shadow * 0.90 + 0.10; // 0%～100% → 50%～100%
		
    }
	
	//------------------------------------------------------------
	// 法線マップ
	//------------------------------------------------------------
    N = Normalmap(N, input.world_position_, uv);

	//------------------------------------------------------------
	// テクスチャカラーを読み込み
	//------------------------------------------------------------
    float4 textureColor = DiffuseTexture.Sample(DiffuseSampler, uv) * input.diffuse_;
    textureColor = saturate(textureColor);
	
    // アルファテスト
    if (textureColor.a < 0.5)
        discard;

	// リニア化 sRGBのテクスチャ → Linearのテクスチャ
    textureColor.rgb = pow(textureColor.rgb, 2.2);
	
    float3 albedo = textureColor.rgb * input.diffuse_.rgb;
	
    output.color0_ = textureColor;
    //output.color0_.rgb = N; // * input.diffuse_;
    //output.color0_.a = 1;
    //return output;
	
	
	
	
	
    float distance = input.position_.w;
	
	
	// LERP: 線形補間 Linear intERPoration
	// result = lerp(A, B, ratio);			0 <= ratio <= 1
	// ※ratioは範囲外になっても動作するが、期待した結果にならない場合はsaturateする
	
    // saturate 飽和 (入力値を0.0	-1.0の間に収める)
	// float n;
	// if (n < 0.0) n = 0.0;
	// if (n > 1.0) n = 1.0;
	// ※saturateは0サイクルで実行可能。コストゼロ

	// abs 絶対値 absolute
	// if(n < 0.0) n = -n;
	// ※absは0サイクルで実行可能。コストゼロ
	// -n;		// 1サイクル
    // abs(n)	// 0サイクル

	// max 最大値
	// min 最小値
	// result = max(a, b);
	// result = min(a, b);
	// aとbの大きい方(または小さい方)を選択する	
	
	// clamp 範囲制限
	// result clamp(x, 0.0, 2.0);
	// 最小値～最大値に収める
	
	
	
	//-------------------------------------------------------------
	// フォグ表現
	//-------------------------------------------------------------
    float ratio = saturate(distance * 0.005);
   //output.color0_.rgb = lerp(output.color0_.rgb, float3(0.4, 0.6, 1), ratio);
	
    int2 position = int2(input.position_.xy); // 画面上のピクセル位置

	//-------------------------------------------------------------
	// CRTスキャンライン表現
	//-------------------------------------------------------------
    // if ((position.y & 1)  == 0)
    //     output.color0_.rgb = 0;
	
	// 市松模様 (checker board)
    //if (( ((position.x >> 4) & 1) ^ ((position.y >> 4) & 1) ) == 1)
    //    output.color0_.rgb = 0;

	// グラデーション	
    //output.color0_.rg = position.xy * float2(1.0 / 1280, 1.0 / 720);
	


	//----------------------------------------------------------
	// RGBの色以外の値を代入してデバッグ表示
	//----------------------------------------------------------
	// output.color0_.rgb = input.world_position_ * 0.1;
	// output.color0_.rgb = normalize(N);
    // output.color0_.rgb = float3(input.uv0_, 0);
	

    
    float3 H = normalize(L + V); // ハーフベクトル
	
	
	

    float3 lightColor = float3(1, 1, 1) * 5; // 光源の明るさ, 色

    float NdotL = saturate(dot(N, L)) + 0.000001;
    float NdotV = saturate(dot(N, V)) + 0.000001;

    float NdotH = saturate(dot(N, H)) + 0.000001;
    float LdotH = saturate(dot(L, H)) + 0.000001;
	
    float roughness = 0.7; // ラフ度 0.0:つるつる ～ 1.0:ざらざら (別名:glossiness, shininess)
    float metallic = 0.5; // 金属度 0.0:非金属   ～ 1.0:金属     (別名:metalness)
	
    float3 specularColor = lerp(float3(0.4, 0.4, 0.4), albedo, metallic);
	
	
	//-------------------------------------------------------------
	// diffuse (ディフューズ) = 拡散反射光
	//-------------------------------------------------------------
	// Lambert 拡散反射光モデル
    // float diffuse = max(0, dot(N, L));
    const float Kd = 1.0 / PI;
    float3 diffuse = lightColor * (saturate(dot(N, L)) * Kd);

	//-------------------------------------------------------------
	// specular (スペキュラー) = 鏡面反射光
	//-------------------------------------------------------------
#if 0
	// Blinn-phong 鏡面反射光モデル
    float shininess = 200.0;
    // float Ks = (shininess + 2.0) / (2.0 * PI);
    float Ks = shininess * (1.0 / (2.0 * PI)) + (2.0 / (2.0 * PI));
    float specular = pow(saturate(dot(N, H)), shininess) * Ks;
	
    specular *= 0.5;	// 強さ調整
#elif 0
	
	// Cook-Torrance BRDFモデル
	//        D * F * G
	// -----------------------
	//  4 * (NdotL) * (NdotV)
	//
	// 照明計算 = (BRDF) * (光の色,強さ) * (NdotL)
	//       Cook-Torrance
    float3 brdf = (D_GGX(N, H, roughness) * F_Schlick(specularColor, NdotV) * G_Smith_Schlick_GGX(roughness, NdotV, NdotL))
	              / //--------------------------------------------------------------------------------------------------------------------
	                                       (4.0 * /*NdotL * */NdotV);
	
    float3 specular = brdf * lightColor /* * NdotL*/;
#else
	// Cook-Torrance BRDF 近似高速化 (Optimizing PBR SIGGRAPH2015)
    float roughness4 = roughness * roughness * roughness * roughness;
    float denominator = (NdotH * NdotH * (roughness4 - 1.0) + 1) * LdotH;
	
    float3 brdf = roughness4 * rcp(4.0 * PI * denominator * denominator * (roughness + 0.5));
	
    float3 specular = brdf * specularColor * lightColor;

#endif	
	
	
	
	
	
	
	
	
	// ambient light 環境光
	// 周辺の明るさを近似
    float3 ambient = float3(0.5, 0.5, 0.5);

	
    output.color0_.rgb *= diffuse + ambient;
    output.color0_.rgb += specular;
    
   //output.color0_.rgb *= shadow;
	
	//==========================================================
	// Image Based Lighting
	//==========================================================
#if 0	
	{
        float3 ibl_diffuse = IBL_Diffuse.SampleLevel(DiffuseSampler, N, 0).rgb * Kd;
        ibl_diffuse *= textureColor.rgb;
		
        output.color0_.rgb = ibl_diffuse;
		
		// 反射ベクトルR
        float3 R = reflect(-V, N);
		
        float mip = roughness * 7;
        float3 ibl_specular = IBL_Specular.SampleLevel(DiffuseSampler, R, mip).rgb;
        float3 environmentBrdf = pow(1.0 - max(roughness, NdotV), 3.0) + specularColor;
        ibl_specular *= environmentBrdf;

        output.color0_.rgb = ibl_diffuse * (1.0 - metallic) + ibl_specular;

		output.color0_.rgb *= 2;
    }
#endif

    //output.color0_.rgb *= 4;
	
	
	//----------------------------------------------------------
	// ガンマ補正 Linear → sRGB
	// ※表示用のガンマカーブ
	//----------------------------------------------------------
	
	// pow べき乗
	// pow(n, x);	nのx乗
    // output.color0_.rgb = pow(output.color0_.rgb, 1.0 / 2.2);
	
	
	
	// 出力パラメータを返す
    return output;
}
