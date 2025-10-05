#pragma once


class Shader;
class ShaderPs;
class ShaderVs;
class ModelSource {
	friend class ModelManager;
	std::string path;
	std::string name;
	int handle = -1;
	bool is_loaded = false;
	physx::PxConvexMesh* convex_mesh = nullptr;
	physx::PxTriangleMesh* triangle_mesh = nullptr;
	MV1_REF_POLYGONLIST ref_poly_{};
	bool polygons_loaded = false;
public:
	MV1_REF_POLYGONLIST* GetPolygon();
	physx::PxConvexMesh* GetOrCreateConvexMesh();
	physx::PxTriangleMesh* GetOrCreateTriangleMesh();
	~ModelSource() { MV1DeleteModel(handle); if (convex_mesh)convex_mesh->release(); if (triangle_mesh)triangle_mesh->release(); }
};

class AnimSource {
	friend class ModelManager;
	std::string path;
	std::string name;
	int handle = -1;
	int index = -1;
	bool is_loaded = false;
public:
	~AnimSource() { MV1DeleteModel(handle); }
};

class Model {
	friend class SceneManager;
	friend class ModelManager;
	friend class ModelRenderer;
	friend class Animator;
	std::string name;
	int handle = -1;
	ModelSource* original = nullptr;

	static inline int instance = 0;
	//TO_DO! シェーダーのポインタはこいつに持たせる(特にトゥーンなどで複数ある場合)
	Shader* shader_ps = nullptr;
	Shader* default_shader_ps = nullptr;
	Shader* shader_vs = nullptr;
	Shader* default_shader_vs = nullptr;

public:
	void SetShader(Shader* pixel = nullptr, Shader* vertex = nullptr);
	void SetDefaultShader(Shader* pixel = nullptr, Shader* vertex = nullptr);
	MV1_REF_POLYGONLIST* GetPolygon() { return original->GetPolygon(); }
	physx::PxConvexMesh* GetConvexMesh();
	physx::PxTriangleMesh* GetTriangleMesh();
	Model();
	Model(const Model& other) {
		name = other.name;
		handle = other.handle;
		SetShader(other.shader_ps, other.shader_vs);
		SetDefaultShader(other.default_shader_ps, other.default_shader_vs);
		instance++;
	}
	~Model() {
		MV1DeleteModel(handle);
		instance--;
	}
	void Draw();
};

class Animation {
	static inline int instance = 0;
	friend class ModelManager;
	friend class Animator;
	std::string name;
	int handle = -1;
	int index = -1;
	int attached_index = -1;
	float total_time = 0;
	float current_time = 0;
	float blend_rate = 0.0f;
	struct AnimationCallBack {
		float ex_frame = 0;
		std::function<void()> function = nullptr;
		bool is_executed = false;
	};
	std::vector<AnimationCallBack> call_backs;
	std::unordered_map<std::string, size_t> method_names;
public:
	Animation() { instance++; }
	Animation(const Animation& other) {
		name = other.name;
		handle = other.handle;
		index = other.index;
		total_time = other.total_time;
		instance++;
	}
	void Update(float speed = 1.0f);
	void InitCallBacks();
	void SetCallBack(std::function<void()>& call_back, float execute_frame, std::string_view method_name);
	void ResetCallBack(std::string_view method_name);
	~Animation() { MV1DeleteModel(handle); instance--; }
};


class ModelManager
{
	ModelManager() = delete;
	ModelManager(const ModelManager&) = delete;
	~ModelManager() = default;
public:
	static std::vector<SafeUniquePtr<ModelSource>> model_chache;
	static std::vector<SafeUniquePtr<AnimSource>> anim_chache;
	struct IndexAndHandle {
		int index = -1;
		int handle = -1;
	};
	static inline int loading_count = 0;
	struct PtrToCacheAndModelData {
		std::vector<SafeUniquePtr<ModelSource>>* model_cache = nullptr;
		std::vector<SafeUniquePtr<AnimSource>>* anim_cache = nullptr;
		SafeUniquePtr<ModelSource> m_source;
		SafeUniquePtr<AnimSource> a_source;
		std::unordered_map<std::string, ModelManager::IndexAndHandle>* m_name = nullptr;
		std::unordered_map<std::string, ModelManager::IndexAndHandle>* m_path = nullptr;
		std::unordered_map<std::string, ModelManager::IndexAndHandle>* a_name = nullptr;
		std::unordered_map<std::string, ModelManager::IndexAndHandle>* a_path = nullptr;
	};
	static std::unordered_map<std::string, IndexAndHandle> m_name;
	static std::unordered_map<std::string, IndexAndHandle> m_path;
	static std::unordered_map<std::string, IndexAndHandle> a_name;
	static std::unordered_map<std::string, IndexAndHandle> a_path;
	static void LoadAsModel(std::string_view path, std::string_view name);
	static void LoadAsAnimation(std::string_view path, std::string_view name);
	static SafeSharedPtr<Model> CloneModelByName(std::string_view name, std::string_view new_name = "");
	static SafeSharedPtr<Model> CloneModelByPath(std::string_view path, std::string_view new_name = "");
	static SafeSharedPtr<Animation> CloneAnimByName(std::string_view name, int index = 0, std::string_view new_name = "");
	static SafeSharedPtr<Animation> CloneAnimByPath(std::string_view path, int index = 0, std::string_view new_name = "");
	static inline const int GetLoadingCount() { return loading_count; }
	static void Init();
	static void Exit();
	static SafeUniquePtr<ShaderPs> default_shader_ps;
	static SafeUniquePtr<ShaderVs> default_shader_vs;



};

