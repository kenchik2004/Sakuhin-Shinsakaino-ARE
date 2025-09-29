//---------------------------------------------------------------------------
//! @file   TextureManager.h
//! @brief  テクスチャ管理
//---------------------------------------------------------------------------
#pragma once

//=========================================================
//! テクスチャの元データ
//=========================================================
class TextureSource {
	friend class TextureManager;

	std::string path;		//!< テクスチャデータの存在するパス
	std::string name;		//!< テクスチャのデータ名
	int handle = -1;		//!< テクスチャハンドル
	bool is_loaded = false;	//!< ロード完了しているかどうか
public:
	//! デストラクタ
	//! 削除と同時にハンドルを開放
	~TextureSource() {
		if (handle >= 0)
			DeleteSharingGraph(handle);
	}
};

//=========================================================
//! テクスチャデータを基にクローンされたデータ
//=========================================================
class Texture {
	friend class TextureManager;
	friend class TextureRenderer;

	std::string name;		//!< テクスチャ名
	int handle = -1;		//!< テクスチャハンドル
public:
	//! デフォルトコンストラクタ
	Texture() = default;

	//! @brief コピーは禁止
	Texture(const Texture& other) = delete;

	//! @brief 削除と同時にハンドルも開放
	~Texture() {
		if (handle >= 0)
			DeleteGraph(handle);			//! 有効かつ元テクスチャがあるなら、テクスチャハンドルを削除
	}

	//! @brief DxLibのドローなどに使う際のハンドルゲッター関数
	inline int GetHandle() { return handle; }
};

//---------------------------------------------------------------------
//! @class テクスチャの管理クラス
//! @details テクスチャの非同期ロード・キャッシュの管理、新たなテクスチャの作成を行うクラス
//---------------------------------------------------------------------
class TextureManager
{
	TextureManager() = delete;							//!< 作成禁止
	TextureManager(const TextureManager&) = delete;		//!< コピー禁止
	~TextureManager() = default;						//!< もちろん削除もできない



	static inline int loading_count = 0;//!< 非同期ロード中テクスチャの数


	//! @brief	テクスチャデータのvectorのインデックスとテクスチャハンドルの構造体
	//!		主にパスやデータ名のマップからキャッシュへのアクセスに使用する
	struct IndexAndHandle {
		int index = -1;		//!<テクスチャキャッシュのどこに元データがあるか
		int handle = -1;	//!<テクスチャハンドル
	};



	static std::vector<SafeUniquePtr<TextureSource>> cache;	//!< キャッシュデータ

	static std::unordered_map<std::string, IndexAndHandle> names;//!<	名前からデータを検索する際に使うデータマップ
	static std::unordered_map<std::string, IndexAndHandle> paths;//!<	パスからデータを検索する際に使うデータのマップ

	//!@brief キャッシュやマップ、それからテクスチャソースへのポインタを持った構造体
	//! 	主に非同期ロードのコールバックでキャッシュに登録を行う際に使う
	struct PtrToCacheAndTextureData {
		std::vector<SafeUniquePtr<TextureSource>>* cache = &TextureManager::cache;	//!< TextureManagerのキャッシュへのポインタ
		std::unordered_map<std::string, IndexAndHandle>* name_map = &TextureManager::names; //!<Texturemanagerのテクスチャ名マップへのポインタ
		std::unordered_map<std::string, IndexAndHandle>* path_map = &TextureManager::paths; //!<Texturemanagerのテクスチャパスマップへのポインタ
		SafeUniquePtr<TextureSource> texture_source;										//!<テクスチャの非同期ロードが終わった後登録するテクスチャデータ
	};


public:

	//! @brief  ロード処理
	//! @param  std::string_view テクスチャのパス
	//! @param  std::string_view テクスチャにつける名前
	//! @details	与えられたパスを基にテクスチャを非同期でロード・キャッシュする
	static void Load(std::string_view path, std::string_view name);

	//! @brief  新規テクスチャ作成処理
	//! @param  std::string_view テクスチャにつける名前
	//! @param  int テクスチャの幅
	//! @param  int テクスチャの高さ
	//! @return		SafeSharedPtr<Texture> 作成したテクスチャのクローン
	//! @details	与えられた縦横サイズのテクスチャ新規で作成・キャッシュする
	static SafeSharedPtr<Texture> Create(std::string_view name, int width, int height);


	//! @brief		キャッシュされたテクスチャを名前から検索->クローンを作って取得する処理
	//! @param		std::string_view ロード時にテクスチャにつけた名前
	//! @param		std::string_view 新たにテクスチャにつける名前(無しなら第一引数と同じ名前になります)
	//! @details	与えられた名前を基にテクスチャを検索しクローンを返す
	//! @return		SafeSharePtr<Texture> 作成したクローンへのshared_ptr
	//! @warning	この関数は予めLoadもしくはCreateしたテクスチャでないとnullptrを返します。先にLoadでキャッシュしてください。
	static SafeSharedPtr<Texture> CloneByName(std::string_view name, std::string_view new_name = "");

	//! @brief		キャッシュされたテクスチャをパスから検索->クローンを作って取得する処理
	//! @param		std::string_view テクスチャのパス
	//! @param		std::string_view 新たにテクスチャにつける名前(無しなら第一引数と同じ名前になります)
	//! @details	与えられた名前を基にテクスチャを検索しクローンを返す
	//! @return		SafeSharePtr<Texture> 作成したクローンへのshared_ptr
	//! @warning	この関数は予めLoadしたテクスチャでないとnullptrを返します。先にLoadでキャッシュしてください。
	//!				また、Createしたテクスチャはファイルパスが存在しないので必ずnullptrが返ります。
	static SafeSharedPtr<Texture> CloneByPath(std::string_view path, std::string_view new_name = "");

	//! @brief		現在非同期ロード中のテクスチャ数を取得する
	static inline const int GetLoadingCount() { return loading_count; }

	static void Init();						//初期化
	static void Exit();						//解放等の終了処理
};

