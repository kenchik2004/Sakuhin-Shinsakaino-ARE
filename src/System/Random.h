#pragma once
namespace Random
{
	//乱数生成用の変数
	static inline unsigned long long seed = 0; //!<乱数生成用のシード値
	//初期化
	inline int Init()
	{
		seed = (unsigned long long)SEC2MICRO(Time::GetOSTimeD()); //!<シード値を初期化
		return 0;
	}
	//乱数生成
	inline int Int(const int& min, const int& max)
	{
		if (min >= max) return min; //範囲が無効な場合は最小値を返す
		seed = (seed * 6364136223846793005ULL + 1) & 0xFFFFFFFFFFFFFFFFULL; //XORShiftアルゴリズム
		return (int)(min + (seed % (max - min + 1))); //指定された範囲の乱数を返す
	}
	//乱数生成(float)
	inline float Range(const float& min, const float& max)
	{
		if (min >= max) return min; //範囲が無効な場合は最小値を返す
		seed = (seed * 6364136223846793005ULL + 1) & 0xFFFFFFFFFFFFFFFFULL; //XORShiftアルゴリズム
		return min + ((float)(seed % (unsigned long long)((max - min) * 1000)) / 1000.0f); //指定された範囲の乱数を返す
	}
	//乱数生成(Vector2)
	inline Vector2 Position(const Vector2& min, const Vector2& max)
	{
		return Vector2(
			Range(min.x, max.x),
			Range(min.y, max.y)
		);
	}
	inline Color Colo(const Color& min, const Color& max)
	{
		return Color(
			Range(min.r, max.r),
			Range(min.g, max.g),
			Range(min.b, max.b),
			Range(min.a, max.a)
		);
	}
	//乱数生成(Vector3)
	inline Vector3 Position(const Vector3& min, const Vector3& max)
	{
		return Vector3(
			Range(min.x, max.x),
			Range(min.y, max.y),
			Range(min.z, max.z)
		);
	}
	//乱数生成(Quaternion)
	inline Quaternion Rotation(const Quaternion& min, const Quaternion& max)
	{
		return Quaternion(
			Range(min.x, max.x),
			Range(min.y, max.y),
			Range(min.z, max.z),
			Range(min.w, max.w)
		);
	}



};

