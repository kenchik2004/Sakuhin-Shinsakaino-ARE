#include "precompile.h"


//通常stringをワイドstringに変換
std::wstring Str2Wstr(std::string in)
{
	// 変換に必要なサイズを取得
	int size_needed = MultiByteToWideChar(CP_ACP, 0, in.c_str(), static_cast<int>(in.size()), NULL, 0);
	// 変換先のwchar_t配列を作成
	std::wstring out(size_needed, 0);
	// 変換を実行
	MultiByteToWideChar(CP_ACP, 0, in.c_str(), static_cast<int>(in.size()), &out[0], size_needed);
	return out;
}

//ワイドstringを通常stringに変換
std::string WStr2Str(std::wstring in)
{

	// 変換に必要なサイズを取得
	int size_needed = WideCharToMultiByte(CP_ACP, 0, in.c_str(), static_cast<int>(in.size()), NULL, 0, NULL, NULL);
	// 変換先のwchar_t配列を作成
	std::string out(size_needed, 0);
	// 変換を実行
	WideCharToMultiByte(CP_ACP, 0, in.c_str(), static_cast<int>(in.size()), &out[0], size_needed, NULL, NULL);
	return out;
}
//通常stringをワイドstring(UTF-8)に変換
std::wstring Str2WstrU8(std::string in)
{
	// 変換に必要なサイズを取得
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, in.c_str(), static_cast<int>(in.size()), NULL, 0);
	// 変換先のwchar_t配列を作成
	std::wstring out(size_needed, 0);
	// 変換を実行
	MultiByteToWideChar(CP_UTF8, 0, in.c_str(), static_cast<int>(in.size()), &out[0], size_needed);
	return out;
}

//ワイドstringを通常string(UTF-8)に変換
std::string WStr2StrU8(std::wstring in)
{

	// 変換に必要なサイズを取得
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, in.c_str(), static_cast<int>(in.size()), NULL, 0, NULL, NULL);
	// 変換先のwchar_t配列を作成
	std::string out(size_needed, 0);
	// 変換を実行
	WideCharToMultiByte(CP_UTF8, 0, in.c_str(), static_cast<int>(in.size()), &out[0], size_needed, NULL, NULL);
	return out;
}
std::string ShiftJISToUTF8(const std::string& shiftJisStr) {
	// Shift_JIS → UTF-16（ワイド文字）
	int wideLen = MultiByteToWideChar(932, 0, shiftJisStr.c_str(), -1, nullptr, 0);
	std::wstring wideStr(wideLen, L'\0');
	MultiByteToWideChar(932, 0, shiftJisStr.c_str(), -1, &wideStr[0], wideLen);

	// UTF-16 → UTF-8
	int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
	std::string utf8Str(utf8Len, '\0');
	WideCharToMultiByte(CP_UTF8, 0, wideStr.c_str(), -1, &utf8Str[0], utf8Len, nullptr, nullptr);

	return utf8Str;
}
SafeSharedPtr<void> CreateInstanceFromName(std::string_view name,TypeInfo& type)
{
	std::string class_name(name);
	const TypeInfo& base_type = type;
	const TypeInfo* p = base_type.Child();
	bool            returnFromTraverse = false;
	const TypeInfo* next = nullptr;

	//----------------------------------------------------------
	// 継承ツリー構造を探索
	// スタック再帰を使わない高速なツリー探索 (stackless tree traversal)
	//----------------------------------------------------------
	while (p && (p != &base_type)) {
		if (!returnFromTraverse) {
			// 名前チェックして一致したら作成
			if (p->ClassName() == class_name) {
				return p->Create();
			}
		}

		if (p->Child() && !returnFromTraverse) {
			// 子がある場合は子を先に調べる。(子から探索で戻ってきた場合は除外)
			next = p->Child();
			returnFromTraverse = false;
		}
		else if (p->Sibling()) {
			// 兄弟がいる場合は兄弟を調べる
			next = p->Sibling();
			returnFromTraverse = false;
		}
		else {
			// 親へ戻る。
			next = p->Parent();
			returnFromTraverse = true;
		}
		p = next;
	}
	return nullptr;
}

std::string_view TypeInfo::ClassName() const
{
	return class_name;
}


size_t TypeInfo::ClassSize() const
{
	return class_size;
}

const TypeInfo* TypeInfo::Parent() const
{
	return parent;
}

const TypeInfo* TypeInfo::Child() const
{
	return child;
}

const TypeInfo* TypeInfo::Sibling() const
{
	return sibling;
}

