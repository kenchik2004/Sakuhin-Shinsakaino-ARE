#pragma once

//---------------------------------------------------------------------------------
//	float2 クラス
//---------------------------------------------------------------------------------
class float2
{
public:
	float x;
	float y;

	float2();									//	コンストラクタ
	float2(float x, float y);

	void clear();								//	ゼロ初期化
	void set(float x, float y);				//	値のセット
	void set(float2& v);

	float GetLength();							//	長さ取得
	void normalize();							//	正規化
	float2 normalized();							//	正規化されたベクトルの取得(ベクトル自体に変化は加えない)
	void SetLength(float length);				//	長さ設定

	float2& operator = (const float2& v);		//	= 演算子のオーバーロード

	float2& operator += (const float2& v);	//	+= 演算子のオーバーロード
	float2& operator -= (const float2& v);	//	-= 演算子のオーバーロード
	float2& operator *= (const float f);		//	*= 演算子のオーバーロード
	float2& operator /= (const float f);		//	/= 演算子のオーバーロード
};

//	+ 演算子のオーバーロード
float2 operator + (const float2& v1, const float2& v2);
//	- 演算子のオーバーロード
float2 operator - (const float2& v1, const float2& v2);
//	* 演算子のオーバーロード
float2 operator * (const float2& v, const float f);
//	/ 演算子のオーバーロード
float2 operator / (const float2& v, const float f);

//	２つの float2 の距離を求める
float GetFloat2Distance(float2& pos1, float2& pos2);
//	２つの float2 の内積を求める
float GetFloat2Dot(float2& v1, float2& v2);
//	２つの float2 の外積を求める
float GetFloat2Cross(float2& v1, float2& v2);
