#pragma once

//---------------------------------------------------------------------------------
//	float3 クラス（ x y z があるクラス）
//---------------------------------------------------------------------------------
class float3
{
public:
	float	x;
	float	y;
	float	z;

	float3();									//	コンストラクタ
	float3(float x, float y, float z);
	float3(VECTOR v);
	float3(Vector3 v);

	void clear();								//	ゼロ初期化
	void set(float x, float y, float z);		//	値のセット
	void set(float3& v);

	float GetLength();							//	長さ取得
	void normalize();							//	正規化
	float3 normalized();						//	正規化されたベクトルの取得
	void SetLength(float length);				//	長さ設定

	float3& operator = (const float3& v);	//	= 演算子のオーバーロード
	float3& operator = (const VECTOR& v);	//	= 演算子のオーバーロード
	float3& operator = (const Vector3& v);	//	= 演算子のオーバーロード

	float3& operator += (const float3& v);	//	+= 演算子のオーバーロード
	float3& operator -= (const float3& v);	//	-= 演算子のオーバーロード
	float3& operator *= (const float f);		//	*= 演算子のオーバーロード
	float3& operator /= (const float f);		//	/= 演算子のオーバーロード
	operator VECTOR() { VECTOR v; v.x = this->x; v.y = this->y;	v.z = this->z;	return v; }
	operator Vector3() { Vector3 v; v.x = this->x; v.y = this->y;	v.z = this->z;	return v; }
};

//	+ 演算子のオーバーロード
float3 operator + (const float3& v1, const float3& v2);
//	- 演算子のオーバーロード
float3 operator - (const float3& v1, const float3& v2);
//	* 演算子のオーバーロード
float3 operator * (const float3& v, const float f);
//	/ 演算子のオーバーロード
float3 operator / (const float3& v, const float f);

//	２つの float3 の距離を求める
float GetFloat3Distance(float3& pos1, float3& pos2);
//	２つの float3 の内積を求める
float GetFloat3Dot(float3& v1, float3& v2);
//	２つの float3 の外積を求める
float3 GetFloat3Cross(float3& v1, float3& v2);
//	float3 を MATRIX で変換したものを返す
float3 GetFloat3VTransform(float3& v, MATRIX& mat);
//	float3のベクトルを平面上に投影
float3 ProjectOnPlane(float3 vec, float3 plane_normal);
inline float3 cast(const Vector3& vec) {
	float3 vec_ = vec;
	return vec_;
}
inline Vector3 operator/(const Vector3& base, const Vector3& denuminator)
{
	Vector3 f_;
	f_.x = physx::PxAbs(denuminator.x) > FLT_EPSILON ? 1.0f / denuminator.x : 0;	// 3項演算子で、処理を気持ち程度軽くする
	f_.y = physx::PxAbs(denuminator.y) > FLT_EPSILON ? 1.0f / denuminator.y : 0;
	f_.z = physx::PxAbs(denuminator.z) > FLT_EPSILON ? 1.0f / denuminator.z : 0;
	return Vector3(base.x * f_.x, base.y * f_.y, base.z * f_.z);
}
inline float3 cast(const VECTOR& vec) {
	float3 vec_ = vec;
	return vec_;
}
inline MATRIX cast(const mat4x4& mat)
{
	return {
		mat.column0.x, mat.column1.x, mat.column2.x, mat.column0.w,
		mat.column0.y, mat.column1.y, mat.column2.y, mat.column1.w,
		mat.column0.z, mat.column1.z, mat.column2.z, mat.column2.w,
		mat.column3.x, mat.column3.y, mat.column3.z, mat.column3.w
	};
}
inline mat4x4 cast(const MATRIX& mat)
{
	return mat4x4(
		Vector4(mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[0][3]).getNormalized(),
		Vector4(mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[1][3]).getNormalized(),
		Vector4(mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[2][3]).getNormalized(),
		Vector4(mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3])
	);
}
// PhysXへの受け渡し用変換
inline Vector3 CastPhysXVec(const float3& v) {
	return Vector3(v.x, v.y, -v.z);
}

// PhysXへの受け渡し用変換
inline Quaternion CastPhysXQuat(const Quaternion& q) {
	return Quaternion(-q.x, -q.y, -q.z, q.w);
}
inline mat4x4 CastPhysXMat(const mat4x4& mat) {
	return mat4x4(
		Vector4(mat.column0.x, mat.column1.x, mat.column2.x, mat.column0.w),
		Vector4(mat.column0.y, mat.column1.y, mat.column2.y, mat.column1.w),
		Vector4(mat.column0.z, mat.column1.z, mat.column2.z, mat.column2.w),
		Vector4(mat.column3.x, mat.column3.y, mat.column3.z, mat.column3.w)
	);
}

inline Vector3 QuaternionToEuler(const Quaternion& q) {
	// Roll (X-axis rotation)
	float roll = RAD2DEG(physx::PxAtan2(2.0f * (q.w * q.x + q.y * q.z), 1.0f - 2.0f * (q.x * q.x + q.y * q.y)));

	// Pitch (Y-axis rotation)
	float pitch = RAD2DEG(physx::PxAsin(2.0f * (q.w * q.y - q.z * q.x)));

	// Yaw (Z-axis rotation)
	float yaw = RAD2DEG(physx::PxAtan2(2.0f * (q.w * q.z + q.x * q.y), 1.0f - 2.0f * (q.y * q.y + q.z * q.z)));

	return Vector3(roll, pitch, yaw);  // X, Y, Z に対応するオイラー角を返す
}
inline Quaternion Inverse(Quaternion q) {
	if (fabsf(q.magnitudeSquared()) < FLT_EPSILON)
		return Quaternion(0, 0, 0, 1);

	float invnorm = 1.0f / q.magnitudeSquared();
	return Quaternion(-q.x * invnorm, -q.y * invnorm, -q.z * invnorm, q.w * invnorm);

}

inline Vector3 Lerp(const Vector3& start, const  Vector3& end, const float& t) {
	Vector3 lerp_v;
	float t_ = physx::PxClamp(t, 0.0f, 1.0f);
	lerp_v.x = physx::PxLerp(start.x, end.x, t_);
	lerp_v.y = physx::PxLerp(start.y, end.y, t_);
	lerp_v.z = physx::PxLerp(start.z, end.z, t_);
	return lerp_v;
}
inline Vector3 LerpUnClamped(const Vector3& start, const  Vector3& end, const float& t) {
	Vector3 lerp_v;
	lerp_v.x = physx::PxLerp(start.x, end.x, t);
	lerp_v.y = physx::PxLerp(start.y, end.y, t);
	lerp_v.z = physx::PxLerp(start.z, end.z, t);
	return lerp_v;
}
inline Quaternion Slerp(const Quaternion& start, const Quaternion& end, const float& t) {
	Quaternion slerp_q;
	slerp_q = physx::PxSlerp(t, start, end);
	return slerp_q;
}

inline Quaternion EulerToQuaternion(const Vector3& euler) {
	Quaternion qx = Quaternion(DEG2RAD(euler.x), Vector3(1, 0, 0));
	Quaternion qy = Quaternion(DEG2RAD(euler.y), Vector3(0, 1, 0));
	Quaternion qz = Quaternion(DEG2RAD(euler.z), Vector3(0, 0, 1));
	return qx * qy * qz;
}