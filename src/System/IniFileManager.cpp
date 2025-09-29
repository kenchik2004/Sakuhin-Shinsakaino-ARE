#include "precompile.h"
#include "IniFileManager.h"
#include "algorithm"

namespace FileSystem {


	bool IniFileManager::GetBool(std::string_view section, std::string_view key, bool default_, std::string path)
	{
		char str[32];
		bool value = default_;
		GetPrivateProfileStringA(section.data(), key.data(), std::to_string(default_).c_str(), str, 32, path.c_str());
		if (str[0] >= '0' && str[0] <= '9') {
			value = std::stoi(str);
			return value;
		}

		if (std::strcmp(str, "true") == 0 || std::strcmp(str, "True") == 0 || std::strcmp(str, "TRUE") == 0)
			value = true;
		else if (std::strcmp(str, "false") == 0 || std::strcmp(str, "False") == 0 || std::strcmp(str, "FLASE") == 0)
			value = false;

		return value;
	}

	int IniFileManager::GetInt(std::string_view section, std::string_view key, int default_, std::string path)
	{

		int value = default_;
		value = GetPrivateProfileIntA(section.data(), key.data(), default_, path.c_str());
		return value;
	}

	float IniFileManager::Getfloat(std::string_view section, std::string_view key, float default_, std::string path)
	{
		float value = default_;
		char str[128];
		GetPrivateProfileStringA(section.data(), key.data(), std::to_string(default_).c_str(), str, 128, path.c_str());
		std::string str_ = str;
		if (std::all_of(&str_[0], &str_[str_.size() - 1], [](auto& a) {return (a >= '0' && a <= '9') || a == '-' || a == '.'; }))
			value = std::stof(str);
		return value;
	}

	Vector2 IniFileManager::GetVector2(std::string_view section, std::string_view key, Vector2 default_, std::string path)
	{
		Vector2 value = default_;
		std::string def_string = std::to_string(default_.x) + ',' + std::to_string(default_.y);
		char str[255];
		GetPrivateProfileStringA(section.data(), key.data(), def_string.c_str(), str, 255, path.c_str());
		std::string str_ = str;
		size_t pos = 0;

		std::vector<std::string> vec;
		std::string num;
		while ((pos = str_.find(',')) != std::string::npos) {
			num = str_.substr(0, pos);
			// 最初のカンマまでを出力
			vec.push_back(num);
			// カンマの次の位置から数値を更新
			str_.erase(0, pos + 1);
		}
		if (str_.size() > 0) {
			num = str_.substr(0, pos);
			vec.push_back(num);
		}
		if (vec.size() > 2)
			vec.erase(vec.begin() + 2, vec.end());
		float vec2[2] = { default_.x,default_.y };
		for (int i = 0; i < vec.size(); i++)
			if (std::all_of(&vec[i][0], &vec[i].back(), [](auto& a) {return (a >= '0' && a <= '9') || a == '-' || a == '.'; }))
				vec2[i] = std::stof(vec[i]);
		value.x = vec2[0];
		value.y = vec2[1];
		return value;
	}

	Vector3 IniFileManager::GetVector3(std::string_view section, std::string_view key, Vector3 default_, std::string path)
	{
		Vector3 value = default_;
		std::string def_string = std::to_string(default_.x) + ',' + std::to_string(default_.y) + ',' + std::to_string(default_.z);
		char str[255];
		GetPrivateProfileStringA(section.data(), key.data(), def_string.c_str(), str, 255, path.c_str());
		std::string str_ = str;
		size_t pos = 0;

		std::vector<std::string> vec;
		std::string num;
		while ((pos = str_.find(',')) != std::string::npos) {
			num = str_.substr(0, pos);
			// 最初のカンマまでを出力
			vec.push_back(num);
			// カンマの次の位置から数値を更新
			str_.erase(0, pos + 1);
		}
		if (str_.size() > 0) {
			num = str_.substr(0, pos);
			vec.push_back(num);
		}
		if (vec.size() > 3)
			vec.erase(vec.begin() + 3, vec.end());
		float vec3[3] = { default_.x,default_.y,default_.z };
		for (int i = 0; i < vec.size(); i++)
			if (std::all_of(&vec[i][0], &vec[i].back(), [](auto& a) {return (a >= '0' && a <= '9') || a == '-' || a == '.'; }))
				vec3[i] = std::stof(vec[i]);
		value.x = vec3[0];
		value.y = vec3[1];
		value.z = vec3[2];
		return value;
	}

	Vector4 IniFileManager::GetVector4(std::string_view section, std::string_view key, Vector4 default_, std::string path)
	{
		Vector4 value = default_;
		std::string def_string = std::to_string(default_.x) + ',' + std::to_string(default_.y) + ',' + std::to_string(default_.z) + ',' + std::to_string(default_.w);
		char str[255];
		GetPrivateProfileStringA(section.data(), key.data(), def_string.c_str(), str, 255, path.c_str());
		std::string str_ = str;
		size_t pos = 0;

		std::vector<std::string> vec;
		std::string num;
		while ((pos = str_.find(',')) != std::string::npos) {
			num = str_.substr(0, pos);
			// 最初のカンマまでを出力
			vec.push_back(num);
			// カンマの次の位置から数値を更新
			str_.erase(0, pos + 1);
		}
		if (str_.size() > 0) {
			num = str_.substr(0, pos);
			vec.push_back(num);
		}
		if (vec.size() > 4)
			vec.erase(vec.begin() + 4, vec.end());
		float vec3[4] = { default_.x,default_.y,default_.z,default_.w };
		for (int i = 0; i < vec.size(); i++)
			if (std::all_of(&vec[i][0], &vec[i].back(), [](auto& a) {return (a >= '0' && a <= '9') || a == '-' || a == '.'; }))
				vec3[i] = std::stof(vec[i]);
		value.x = vec3[0];
		value.y = vec3[1];
		value.z = vec3[2];
		value.w = vec3[3];
		return value;
	}


	Quaternion IniFileManager::GetQuaternion(std::string_view section, std::string_view key, Quaternion default_, std::string path)
	{
		Quaternion value = default_;
		std::string def_string = std::to_string(default_.x) + ',' + std::to_string(default_.y) + ',' + std::to_string(default_.z) + ',' + std::to_string(default_.w);
		char str[255];
		GetPrivateProfileStringA(section.data(), key.data(), def_string.c_str(), str, 255, path.c_str());
		std::string str_ = str;
		size_t pos = 0;

		std::vector<std::string> vec;
		std::string num;
		while ((pos = str_.find(',')) != std::string::npos) {
			num = str_.substr(0, pos);
			// 最初のカンマまでを出力
			vec.push_back(num);
			// カンマの次の位置から数値を更新
			str_.erase(0, pos + 1);
		}
		if (str_.size() > 0) {
			num = str_.substr(0, pos);
			vec.push_back(num);
		}
		if (vec.size() > 4)
			vec.erase(vec.begin() + 4, vec.end());
		float quat[4] = { default_.x,default_.y,default_.z,default_.w };
		for (int i = 0; i < vec.size(); i++)
			if (std::all_of(&vec[i][0], &vec[i].back(), [](auto& a) {return (a >= '0' && a <= '9') || a == '-' || a == '.'; }))
				quat[i] = std::stof(vec[i]);
		value.x = quat[0];
		value.y = quat[1];
		value.z = quat[2];
		value.w = quat[3];
		return value;
	}

	std::string IniFileManager::GetString(std::string_view section, std::string_view key, std::string default_, std::string path)
	{
		char str[1024];
		GetPrivateProfileStringA(section.data(), key.data(), default_.c_str(), str, 1024, path.c_str());
		return str;
	}

	void IniFileManager::SetBool(std::string_view section, std::string_view key, bool value, std::string path)
	{
		std::string value_str = value ? "true" : "false";
		WritePrivateProfileStringA(section.data(), key.data(), value_str.c_str(), path.c_str());
	}

	void IniFileManager::SetInt(std::string_view section, std::string_view key, int value, std::string path)
	{
		std::string value_str = std::to_string(value);
		WritePrivateProfileStringA(section.data(), key.data(), value_str.c_str(), path.c_str());
	}

	void IniFileManager::SetFloat(std::string_view section, std::string_view key, float value, std::string path)
	{
		std::string value_str = std::to_string(value);
		WritePrivateProfileStringA(section.data(), key.data(), value_str.c_str(), path.c_str());
	}

	void IniFileManager::SetVector2(std::string_view section, std::string_view key, Vector2 value, std::string path)
	{
		std::string str_x = std::to_string(value.x);
		std::string str_y = std::to_string(value.y);
		std::string value_str = str_x.substr(0, str_x.find('.') + 4) + ',';
		value_str += str_y.substr(0, str_y.find('.') + 4);
		WritePrivateProfileStringA(section.data(), key.data(), value_str.c_str(), path.c_str());

	}

	void IniFileManager::SetVector3(std::string_view section, std::string_view key, Vector3 value, std::string path)
	{
		std::string str_x = std::to_string(value.x);
		std::string str_y = std::to_string(value.y);
		std::string str_z = std::to_string(value.z);
		std::string value_str = str_x.substr(0, str_x.find('.') + 4) + ',';
		value_str += str_y.substr(0, str_y.find('.') + 4) + ',';
		value_str += str_z.substr(0, str_z.find('.') + 4);
		WritePrivateProfileStringA(section.data(), key.data(), value_str.c_str(), path.c_str());
	}

	void IniFileManager::SetVector4(std::string_view section, std::string_view key, Vector4 value, std::string path)
	{
		std::string str_x = std::to_string(value.x);
		std::string str_y = std::to_string(value.y);
		std::string str_z = std::to_string(value.z);
		std::string str_w = std::to_string(value.w);
		std::string value_str = str_x.substr(0, str_x.find('.') + 4) + ',';
		value_str += str_y.substr(0, str_y.find('.') + 4) + ',';
		value_str += str_z.substr(0, str_z.find('.') + 4) + ',';
		value_str += str_w.substr(0, str_w.find('.') + 4);
		WritePrivateProfileStringA(section.data(), key.data(), value_str.c_str(), path.c_str());

	}

	void IniFileManager::SetQuaternion(std::string_view section, std::string_view key, Quaternion value, std::string path)
	{
		Vector4 vec3 = { value.x,value.y ,value.z ,value.w };
		SetVector4(section, key, vec3, path);
	}

	void IniFileManager::SetString(std::string_view section, std::string_view key, std::string value, std::string path)
	{

		WritePrivateProfileStringA(section.data(), key.data(), value.c_str(), path.c_str());
	}


}