#pragma once
#define DEFAULT_EXCEPTION_PARAM __FILE__,__LINE__,__FUNCTION__
#define INTEGER_NAME(integer) #integer


//デフォルトの例外検知用クラス
class Exception
{
public:
	//マクロ DEFAULT_EXCEPTION_PARAMを使用して初期化ができる
	//引数：
	//	メインメッセージの内容
	//	例外が発生したファイルの名前
	//	例外が発生した行番号 
	//	例外が発生した関数名
	Exception(const char* main_message, const char* file_name, int line, const char* func_name);
	void Show();
protected:
	bool is_assert = false;
	Exception() {}
	std::string message = "";//最終的に出力するメッセージ
};

//nullptrへのアクセス検知用例外クラス
class NullptrException :public Exception {
public:
	NullptrException() = delete;		//規定コンストラクタの削除
	//マクロ　INTEGER_NAME と DEFAULT_EXCEPTION_PARAMを使用して初期化ができる
	//引数：
	//	アクセスしようとした変数名
	//	例外が発生したファイルの名前
	//	例外が発生した行番号 
	//	例外が発生した関数名
	NullptrException(const char* integer_name, const char* file_name, int line, const char* func_name);
	NullptrException(const char* message_);
};

//配列外アクセス検知用例外クラス
class OutOfRangeException :public Exception {
public:
	OutOfRangeException() = delete;		//規定コンストラクタの削除
	//マクロ　INTEGER_NAME と DEFAULT_EXCEPTION_PARAMを使用して初期化ができる
	//引数：
	//	アクセスしようとしたインデックス
	//	アクセスしようとした配列のサイズ
	//	アクセスしようとした配列の名前
	//	例外が発生したファイルの名前
	//	例外が発生した行番号 
	//	例外が発生した関数名
	OutOfRangeException(int index, int array_size, const char* array_name, const char* file_name, int line, const char* func_name);
};
class MemoryLeakException :public Exception {
public:
	MemoryLeakException() = delete;		//規定コンストラクタの削除
	//マクロ　INTEGER_NAME と DEFAULT_EXCEPTION_PARAMを使用して初期化ができる
	//引数：
	//	解放できなかった変数の名前(またはオブジェクト・コンポーネント名)
	//	例外が発生したファイルの名前
	//	例外が発生した行番号 
	//	例外が発生した関数名
	MemoryLeakException(const char* integer_name, const char* file_name, int line, const char* func_name);
};

