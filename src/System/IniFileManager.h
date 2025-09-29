#pragma once
namespace FileSystem {

	class IniFileManager final
	{
	public:
		static bool GetBool(std::string_view section, std::string_view key, bool default_, std::string path);
		static int GetInt(std::string_view section, std::string_view key, int default_, std::string path);
		static float Getfloat(std::string_view section, std::string_view key, float default_, std::string path);
		static Vector2 GetVector2(std::string_view section, std::string_view key, Vector2 default_, std::string path);
		static Vector3 GetVector3(std::string_view section, std::string_view key, Vector3 default_, std::string path);
		static Vector4 GetVector4(std::string_view section, std::string_view key, Vector4 default_, std::string path);
		static Quaternion GetQuaternion(std::string_view section, std::string_view key, Quaternion default_, std::string path);
		static std::string GetString(std::string_view section, std::string_view key, std::string default_, std::string path);

		static void SetBool(std::string_view section, std::string_view key, bool value, std::string path);
		static void SetInt(std::string_view section, std::string_view key, int value, std::string path);
		static void SetFloat(std::string_view section, std::string_view key, float value, std::string path);
		static void SetVector2(std::string_view section, std::string_view key, Vector2 value, std::string path);
		static void SetVector3(std::string_view section, std::string_view key, Vector3 value, std::string path);
		static void SetVector4(std::string_view section, std::string_view key, Vector4 value, std::string path);
		static void SetQuaternion(std::string_view section, std::string_view key, Quaternion value, std::string path);
		static void SetString(std::string_view section, std::string_view key, std::string value, std::string path);

	private:


	};

}
