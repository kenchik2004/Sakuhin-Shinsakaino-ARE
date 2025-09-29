#include "precompile.h"
#include "Exception.h"
#include "assert.h"
#include <codecvt>
#include <deque>


void Log(std::string log) {
	static std::deque<std::string> log_lines;

	log_lines.emplace_back(log);
	if (log_lines.size() > 1000) // 履歴保持数制限
		log_lines.pop_front();
#if 0
	//	ImGui::Begin("console");
	for (auto& log : log_lines) {
		ImGui::TextColored(ImVec4(1, 0, 0, 1), ShiftJISToUTF8(log).c_str());
	}
	//	ImGui::End();
#endif
}

Exception::Exception(const char* main_message, const char* file_name, int line, const char* func_name)
{
	message = "ファイル名: ";
	message += file_name;
	message += "\n行: ";
	message += std::to_string(line);
	message += "\n関数名: ";
	message += func_name;
	message += "\nで例外スロー: \n";
	message += main_message;
}

void Exception::Show()
{

	printfDx(message.c_str());
	printfDx("\n");
	//#ifndef PACKAGE_BUILD
		//Log(message);
	//#endif
#ifndef NDEBUG
	std::wstring wstr = Str2Wstr(message);
	if (is_assert)
		_wassert(wstr.c_str(), __FILEW__, __LINE__);
#else
	if (is_assert)
		_ASSERT(false, "ERROR!!");
#endif


}

NullptrException::NullptrException(const char* integer_name, const char* file_name, int line, const char* func_name)
{
	message = "ファイル名: ";
	message += file_name;
	message += "\n行: ";
	message += std::to_string(line);
	message += "\n関数名: ";
	message += func_name;
	message += "\nで例外スロー: \n";
	message += integer_name;
	message += " がnullptrでした。\n";

}

NullptrException::NullptrException(const char* message_)
{
	message += message_;
}

OutOfRangeException::OutOfRangeException(int index, int array_size, const char* array_name, const char* file_name, int line, const char* func_name)
{
	message = "ファイル名: ";
	message += file_name;
	message += "\n行: ";
	message += std::to_string(line);
	message += "\n関数名: ";
	message += func_name;
	message += "\nで例外スロー: \n";
	message += std::to_string(index);
	message += "は、";
	message += array_name;
	message += "のサイズ [";
	message += std::to_string(array_size);
	message += "] を超えています。\n";
}

MemoryLeakException::MemoryLeakException(const char* integer_name, const char* file_name, int line, const char* func_name)
{
	is_assert = true;
	message = "ファイル名: ";
	message += file_name;
	message += "\n行: ";
	message += std::to_string(line);
	message += "\n関数名: ";
	message += func_name;
	message += "\nで例外スロー: \n";
	message += integer_name;
	message += "が解放できませんでした。循環参照が起こっている可能性があります。\n 変数をstd::weak_ptrで保持、メモリ解放時にstd::shared_ptrにnullptrを代入などを試してみてください。";
}
