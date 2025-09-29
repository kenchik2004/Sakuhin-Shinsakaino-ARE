#pragma once

USING_PTR(Scene);
USING_PTR(DontDestroyOnLoadScene);
//@brief シーンマネジメントクラス
class SceneManager final
{

	static SafeSharedPtr<Model> debug_box;	//!< デバッグ用ボックス(SooS提供)のハンドル

public:
	//-----------------------------
	// Initブロック(初期化処理)
	//-----------------------------

	static int Init();		//<初期化

	//-----------------------------

	//-----------------------------
	// Physicsブロック(物理前後処理)
	//-----------------------------

	static void PrePhysics();		//<物理前更新
	static void Physics();			//<物理更新
	static void PostPhysics();		//<物理後更新


	//-----------------------------

	//-----------------------------
	// Updateブロック(更新前後処理)
	//-----------------------------
	static void PreUpdate();		//<前更新
	static void Update();			//<更新
	static void LateUpdate();		//<遅延更新
	static void PostUpdate();		//<後更新

	//-----------------------------

	//-----------------------------
	// Drawブロック(描画前後処理)
	//-----------------------------

private:
	static void DrawCycleForOneScene(SceneP scene);		//!< 1シーンに対して一括でPreDraw,Draw,LateDrawを行うので使いまわし用の関数
public:
	static void Draw();				//<描画
	static void DebugDraw();		//<デバッグ描画
	static void LateDebugDraw();	//<後デバッグ描画

	//ここの処理は次フレームまで反映されない
	static void PostDraw();			//<フレーム中最終更新

	static void Exit();			//<終了


	//裏のデフォルトシーンを取得
	static inline SceneP GetDontDestoryOnLoadScene() { return another_scenes[0]; }

	//特定のシーンを取得
	template<class T, std::enable_if_t<std::is_convertible_v<T*, Scene*>, int> = 0> static inline SafeSharedPtr<T> GetScene(bool include_another = false)
	{
		ClassTypeInfo<T>& info = T::info;

		SafeSharedPtr<T> scene_pick = nullptr;
		//裏シーンも含める
		std::vector<SceneP> all_scenes = scenes;
		//検索先に裏シーンを追加(引数で裏シーンを含める場合のみ)
		if (include_another)
			for (auto& another_scene : another_scenes)
				all_scenes.push_back(another_scene);

		//作成時のクラス名が一致するシーンを検索
		for (auto& ite : all_scenes) {
			if (ite->status.ClassName() == info.ClassName())
				scene_pick = SafeStaticCast<T>(ite);

		}
		return scene_pick;

	}

	//現在の表シーンを取得
	static inline SceneP GetCurrentScene() {
		return current_scene;
	}

	template<class T, std::enable_if_t<std::is_convertible_v<T*, Scene*>, int> = 0 > static inline void Change(SafeSharedPtr<T> scene)
	{
		//シーンの切り替え
		//カレントシーンがいないならそのままロード
		if (!current_scene) {
			current_scene = scene;
			scene->Init();
			//ロード中にdeltatimeが蓄積し、物理がぶっ壊れることがあるため時飛ばし
			Time::ResetTime();
			return;
		}
		//カレントシーンがいる場合は終了してロード
		current_scene->Exit();
		current_scene->Destroy();
		current_scene = scene;
		scene->Init();
		//ロード中にdeltatimeが蓄積し、物理がぶっ壊れることがあるため時飛ばし
		Time::ResetTime();

	}

	//特定のシーンを完全に破棄
	template <class T, std::enable_if_t<std::is_convertible_v<T*, Scene*>, int> = 0 > static inline void Destroy(SafeSharedPtr<T> destroy_scene) {


		//シーンを検索
		for (auto scene = scenes.begin(); scene != scenes.end();) {
			//見つかったら破棄
			if ((*scene) == destroy_scene) {
				(*scene)->Exit();
				(*scene)->UnLoad();

				//シーンを破棄する際、リークを検知するためtry->catch
				try {
					(*scene)->Destroy();
					(*scene)->DestroyPhysics();
				}
				catch (Exception& ex) {
					ex.Show();
					break;
				}
				scene = scenes.erase(scene);
				if (destroy_scene != current_scene)
					destroy_scene.reset();
				break;
			}
		}
		//破棄シーンがカレントシーンなら、参照を破棄
		if (destroy_scene == current_scene) {
			current_scene.reset();
		}

	}

	//シーンをロード(作成)
	template<class T, std::enable_if_t<std::is_convertible_v<T*, Scene*>, int> = 0, class...Args> static inline SafeSharedPtr<T> Load(SafeSharedPtr<T> ptr = nullptr, Args&&...args)
	{
		//シーンの作成(いたらロードの必要なし)
		if (SafeSharedPtr<T> scene = GetScene<T>()) {
			//指定のシーンに変更
			Change(SafeStaticCast<Scene>(scene));
			//変更先シーンのポインタを返す
			return scene;

		}

		if (!SafeDynamicCast<Scene>(ptr))
			return SafeSharedPtr<T>(nullptr);
		//(いなかったら、ロードの必要あり)
		SafeSharedPtr<Scene> scene;
		//ただし、CreateInstance等でnewされたポインタがあれば、そいつをmake_sharedせず直接Sharedに渡して登録する
		if (ptr)
			scene = SafeStaticCast<Scene>(ptr);
		else
			scene = make_safe_shared<T>(std::forward<Args>(args)...);
		//シーンの作成&登録
		scene->Construct();
		//シーンの配列に登録
		scenes.push_back(scene);
		//ロードデータがあるならロード
		scene->Load();
		//シーン変更
		Change(scene);

		return SafeStaticCast<T>(scene);
	}
	//裏シーンをロード
	template <class T, std::enable_if_t<std::is_convertible_v<T*, Scene*>, int> = 0, class... Args> static inline SafeSharedPtr<T> LoadAsAnother(Args&&... args) {
		//シーンの作成(いたらロードの必要なし)
		if (SafeSharedPtr<T> scene = GetScene<T>()) {
			//見つかったシーンのポインタを返す
			return scene;

		}

		//(いなかったら、ロードの必要あり)
		auto scene = make_safe_shared<T>(std::forward<Args>(args)...);
		//シーンの作成&登録
		scene->Construct();
		//ロードデータがあるならロード
		scene->Load();
		//シーンの配列に登録
		another_scenes.push_back(SafeStaticCast<Scene>(scene));
		scene->Init();

		return scene;
	}

	//オブジェクト関係の操作用クラス
	class Object {
	public:

		//オブジェクトの作成(シーン指定可)
		template<class T, std::enable_if_t<std::is_convertible_v<T*, ::Object*>, int> = 0, class...Args>
		static inline SafeSharedPtr<T> Create(SceneP target_scene = nullptr, Args&&...args)
		{
			if (!current_scene && !target_scene)
				return nullptr;
			SafeSharedPtr<T> obj;
			if (target_scene)
				obj = target_scene->CreateGameObject<T>(typeid(T).name() + 6, std::forward<Args>(args)...);
			else
				obj = current_scene->CreateGameObject<T>(typeid(T).name() + 6, std::forward<Args>(args)...);


			return obj;
		}

		//オブジェクトの作成(名付け)
		template<class T, std::enable_if_t<std::is_convertible_v<T*, ::Object*>, int> = 0, class...Args>
		static inline SafeSharedPtr<T> Create(std::string_view name_, Args&&...args)
		{
			//カレントシーンがいないなら何もせずリターン
			if (!current_scene)
				return nullptr;
			//カレントシーンにオブジェクト作成を依頼
			SafeSharedPtr<T> obj = current_scene->CreateGameObject<T>(name_, std::forward<Args>(args)...);
			//作成したオブジェクトをその場で初期化


			//初期化後のオブジェクトを返す
			return obj;
		}


		//オブジェクト取得(シーン指定可)
		template<class T, std::enable_if_t<std::is_convertible_v<T*, ::Object*>, int> = 0> static inline SafeSharedPtr<T> Get(SceneP target_scene = nullptr) {

			//カレントシーンもターゲットシーンもいない場合、nullptrを返す
			if (!current_scene && !target_scene)
				return nullptr;


			SafeSharedPtr<T> obj = nullptr;
			//シーン指定をしている場合
			if (target_scene)
			{
				//そのシーンからオブジェクトを検索する
				obj = target_scene->GetGameObjectPtr<T>();

				return obj;
			}
			//指定がない場合はカレントシーンから検索
			obj = current_scene->GetGameObjectPtr<T>();
			return obj;
		}

		//オブジェクトをタグから検索
		template<class T, std::enable_if_t<std::is_convertible_v<T*, ::Object*>, int> = 0> static inline SafeSharedPtr<T> GetWithTag(::Object::TAG tag) {
			//カレントシーンがいないならnullptrを返す
			if (!current_scene)
				return nullptr;
			//カレントシーンからオブジェクトを検索
			SafeSharedPtr<T> obj = current_scene->GetGameObjectPtr<T>(tag);

			//見つかったオブジェクトを返す
			return obj;
		}

		//オブジェクトを名前から検索
		template<class T, std::enable_if_t<std::is_convertible_v<T*, ::Object*>, int> = 0> static inline SafeSharedPtr<T> Get(std::string_view name_) {
			//カレントシーンがいないならnullptrを返す
			if (!current_scene)
				return nullptr;
			//カレントシーンからオブジェクトを検索
			SafeSharedPtr<T> obj = current_scene->GetGameObjectPtr<T>(name_);

			//見つかったオブジェクトを返す
			return obj;
		}


		//同じ型のオブジェクトをまとめて取得
		template<class T, std::enable_if_t<std::is_convertible_v<T*, ::Object*>, int> = 0> static inline std::vector<SafeSharedPtr<T>> GetArray() {

			//空っぽの配列を用意
			std::vector<SafeSharedPtr<T>> vec(0);
			//カレントシーンがいないなら、空のまま返す
			if (!current_scene)
				return vec;

			//カレントシーンから、まとめて検索
			vec = current_scene->GetGameObjectPtrVec<T>();

			//見つかったものを返す
			return vec;
		}


		//同じ型のオブジェクトをまとめて検索
		template<class T> static inline std::vector<SafeSharedPtr<T>> GetArrayWithTag(::Object::TAG tag) {

			//空っぽの配列を用意
			std::vector<SafeSharedPtr<T>> vec(0);
			//カレントシーンがいないなら、空っぽのまま返す
			if (!current_scene)
				return vec;

			//カレントシーンからまとめて検索
			vec = current_scene->GetGameObjectPtrVec<T>(tag);

			//見つかったものを返す
			return vec;
		}

		//オブジェクトを削除
		static inline void Destory(ObjectP destroy_obj) {
			//カレントシーンがいないなら何もしない
			if (!current_scene)
				return;

			//カレントシーンからオブジェクトを削除
			current_scene->DestroyGameObject(destroy_obj);
			//削除後、参照を削除
			destroy_obj.reset();
		}
		//オブジェクトを削除(シーン指定あり)
		static inline void Destory(SceneP target_scene, ObjectP destroy_obj) {
			//シーンがいないなら何もしない(そもそも大問題)
			if (!target_scene)
				return;

			//カレントシーンからオブジェクトを削除
			target_scene->DestroyGameObject(destroy_obj);
			//削除後、参照を削除
			destroy_obj.reset();
		}

		//シーン切り替え時に破棄しないオブジェクトを登録(デフォルト裏シーンに所有権譲渡)
		static void DontDestroyOnLoad(ObjectP obj, SceneP from_where);
	};


	static std::vector<std::function<void()>> func_on_loop_finish;
private:
	static ScenePVec scenes;			//!<作成済みシーンの配列
	static ScenePVec another_scenes;	//!<裏シーンの配列
	static SceneP current_scene;		//!<現在シーン(カレントシーン)へのポインタ
};

