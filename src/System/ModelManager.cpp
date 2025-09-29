#include "precompile.h"
#include "ModelManager.h"
#include "System/Shader.h"

using namespace physx;

std::vector<SafeUniquePtr<ModelSource>>	ModelManager::model_chache;
std::vector<SafeUniquePtr<AnimSource>>	ModelManager::anim_chache;
std::unordered_map<std::string, ModelManager::IndexAndHandle>	ModelManager::m_name;
std::unordered_map<std::string, ModelManager::IndexAndHandle>	ModelManager::m_path;
std::unordered_map<std::string, ModelManager::IndexAndHandle>	ModelManager::a_name;
std::unordered_map<std::string, ModelManager::IndexAndHandle>	ModelManager::a_path;
SafeUniquePtr<ShaderPs> ModelManager::default_shader_ps = nullptr;
SafeUniquePtr<ShaderVs> ModelManager::default_shader_vs = nullptr;


void ModelManager::LoadAsModel(std::string_view path, std::string_view name)
{
	std::string path_key(path);
	std::string name_key(name);
	if (m_path.count(path_key) || m_name.count(name_key))
		return;
	auto model = make_safe_unique<ModelSource>();
	model->name = name_key;
	model->path = path_key;
	auto&& call_back = [](int mv1_handle, void* data) {
		auto ptr = reinterpret_cast<PtrToCacheAndModelData*>(data);
		ptr->m_source->is_loaded = true;
		int cache_index = ptr->model_cache->size();
		auto name = ptr->m_source->name;
		auto path = ptr->m_source->path;
		ptr->model_cache->push_back(std::move(ptr->m_source));
		(*(ptr->m_name))[name].index = cache_index;
		(*(ptr->m_path))[path].index = cache_index;
		loading_count--;
		delete ptr;
		};
	PtrToCacheAndModelData* data = new PtrToCacheAndModelData;
	data->model_cache = &model_chache;
	data->m_name = &m_name;
	data->m_path = &m_path;
	data->m_source = std::move(model);
	SetUseASyncLoadFlag(true);
	data->m_source->handle = MV1LoadModel(path_key.c_str());
	m_name[name_key].handle = data->m_source->handle;
	m_path[path_key].handle = data->m_source->handle;
	SetASyncLoadFinishCallback(data->m_source->handle, call_back, data);
	loading_count++;
	SetUseASyncLoadFlag(false);
	return;
}

void ModelManager::LoadAsAnimation(std::string_view path, std::string_view name)
{
	std::string path_key(path);
	std::string name_key(name);
	if (a_path.count(path_key) || a_name.count(name_key))
		return;
	auto anim = make_safe_unique<AnimSource>();
	anim->name = name_key;
	anim->path = path_key;
	auto call_back = [](int mv1_handle, void* data) {
		auto ptr = reinterpret_cast<PtrToCacheAndModelData*>(data);
		ptr->a_source->is_loaded = true;
		int cache_index = ptr->anim_cache->size();
		auto name = ptr->a_source->name;
		auto path = ptr->a_source->path;
		ptr->anim_cache->push_back(std::move(ptr->a_source));
		(*(ptr->a_name))[name].index = cache_index;
		(*(ptr->a_path))[path].index = cache_index;
		loading_count--;
		delete ptr;
		};
	PtrToCacheAndModelData* data = new PtrToCacheAndModelData;
	data->anim_cache = &anim_chache;
	data->a_name = &a_name;
	data->a_path = &a_path;
	data->a_source = std::move(anim);
	SetUseASyncLoadFlag(true);
	data->a_source->handle = MV1LoadModel(path_key.c_str());
	a_name[name_key].handle = data->a_source->handle;
	a_path[path_key].handle = data->a_source->handle;
	SetASyncLoadFinishCallback(data->a_source->handle, call_back, data);
	loading_count++;
	SetUseASyncLoadFlag(false);
}

SafeSharedPtr<Model> ModelManager::CloneModelByName(std::string_view name, std::string_view new_name)
{
	std::string name_(name);
	SafeSharedPtr<Model> model = nullptr;
	auto resource = m_name.find(name_);
	if (resource != m_name.end() && resource->second.handle >= 0) {
		model = make_safe_shared<Model>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		while (resource->second.index < 0) {}
		model->handle = MV1DuplicateModel(model_chache[resource->second.index]->handle);
		model->name = new_name == "" ? model_chache[resource->second.index]->name : new_name;
		model->original = model_chache[resource->second.index].raw_unique().get();
		model->default_shader_ps = default_shader_ps.raw_unique().get();
		model->default_shader_vs = default_shader_vs.raw_unique().get();
	}

	return model;
}

SafeSharedPtr<Model> ModelManager::CloneModelByPath(std::string_view path, std::string_view new_name)
{
	std::string path_(path);
	SafeSharedPtr<Model> model = nullptr;
	auto resource = m_path.find(path_);
	if (resource != m_path.end() && resource->second.handle >= 0) {
		model = make_safe_shared<Model>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		while (resource->second.index < 0) {}
		model->handle = MV1DuplicateModel(model_chache[resource->second.index]->handle);
		model->name = new_name == "" ? model_chache[resource->second.index]->name : new_name;
		model->original = model_chache[resource->second.index].raw_unique().get();
		model->default_shader_ps = default_shader_ps.raw_unique().get();
		model->default_shader_vs = default_shader_vs.raw_unique().get();
	}

	return model;
}

SafeSharedPtr<Animation> ModelManager::CloneAnimByName(std::string_view name, int index, std::string_view new_name)
{
	std::string name_(name);
	SafeSharedPtr<Animation> anim = nullptr;
	auto resource = a_name.find(name_);
	if (resource != a_name.end() && resource->second.handle >= 0) {
		anim = make_safe_shared<Animation>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		anim->handle = MV1DuplicateModel(anim_chache[resource->second.index]->handle);
		anim->total_time = MV1GetAnimTotalTime(anim->handle, index);
		anim->name = new_name == "" ? anim_chache[resource->second.index]->name : new_name;
		anim->index = index;
	}

	return anim;
}

SafeSharedPtr<Animation> ModelManager::CloneAnimByPath(std::string_view path, int index, std::string_view new_name)
{
	std::string path_(path);
	SafeSharedPtr<Animation> anim = nullptr;
	auto resource = a_path.find(path_);
	if (resource != a_path.end() && resource->second.handle >= 0) {
		anim = make_safe_shared<Animation>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		anim->handle = MV1DuplicateModel(anim_chache[resource->second.index]->handle);
		anim->total_time = MV1GetAnimTotalTime(anim->handle, anim->index);
		anim->name = new_name == "" ? anim_chache[resource->second.index]->name : new_name;
		anim->index = index;
	}

	return anim;
}

void ModelManager::Init()
{
	default_shader_ps = make_safe_unique<ShaderPs>("data/shader/ps_model.fx");
	default_shader_vs = make_safe_unique<ShaderVs>("data/shader/vs_model.fx", 8);
}

void ModelManager::Exit()
{
	WaitHandleASyncLoadAll();
	model_chache.clear();
	anim_chache.clear();
	m_name.clear();
	m_path.clear();
	a_name.clear();
	a_path.clear();
	default_shader_ps.reset();
	default_shader_vs.reset();
#ifndef PACKAGE_BUILD
	if (Model::instance > 0) {
		std::string msg = typeid(Model).name();
		throw(MemoryLeakException(msg.c_str(), DEFAULT_EXCEPTION_PARAM));
	}
	if (Animation::instance > 0) {
		std::string msg = typeid(Animation).name();
		throw(MemoryLeakException(msg.c_str(), DEFAULT_EXCEPTION_PARAM));
	}
#endif

}

MV1_REF_POLYGONLIST* ModelSource::GetPolygon()
{
	if (!polygons_loaded) {
		MV1SetupReferenceMesh(handle, -1, false);
		ref_poly_ = MV1GetReferenceMesh(handle, -1, false);
	} return &ref_poly_;
}

physx::PxConvexMesh* ModelSource::GetOrCreateConvexMesh()
{
	if (!convex_mesh) {
		PxCookingParams params(PhysicsManager::GetPhysicsInstance()->getTolerancesScale());
#ifndef PACKAGE_BUILD
		if (handle < 0)
			throw(Exception("凸メッシュ作成に失敗しました:モデルデータが無効です。モデルパスが有効か確認してください。", DEFAULT_EXCEPTION_PARAM));
#endif
		GetPolygon();

		PxConvexMeshDesc  desc;
		// 頂点データ取得
		std::vector<PxVec3> vertices;
		auto meshroot_mat = MV1GetFrameBaseLocalMatrix(handle, 0);
#if 1
		for (int i = 0; i < ref_poly_.VertexNum; i++) {

			VECTOR vertex = VTransform(ref_poly_.Vertexs[i].Position, meshroot_mat);
			Vector3 vert = Vector3(vertex.x, vertex.y, vertex.z);
			vertices.push_back(vert);
			ref_poly_.Vertexs[i].Position = vertex;

		}
#if 0
		{

			std::ofstream ostream("data/dice/vertex.txt");
			if (!ostream.fail())
			{

				ostream.clear();
				ostream << "[vertex]\n\n";
				for (int i = 0; i < vertices.size(); i++) {
					std::string str;
					str += "{";
					str += std::to_string(vertices[i].x) + "," + std::to_string(vertices[i].y) + "," + std::to_string(vertices[i].z) + "},\n";

					ostream << str.c_str();

				}
				ostream << "\n";
				ostream.close();
			}
		}
#endif


#endif



		// インデックスデータ取得
		std::vector<PxU32> indices;
		for (int i = 0; i < ref_poly_.PolygonNum; i++) {
			PxU32 idx0 = ref_poly_.Polygons[i].VIndex[0];
			PxU32 idx1 = ref_poly_.Polygons[i].VIndex[1];
			PxU32 idx2 = ref_poly_.Polygons[i].VIndex[2];

			indices.push_back(idx0);
			indices.push_back(idx1);
			indices.push_back(idx2);
		}
#if 0
		{
			std::ofstream ostream("data/dice/vertex.txt", std::ios::app);
			if (!ostream.fail())
			{

				ostream << "[index]\n\n";
				for (int i = 0; i < indices.size() - 2; i += 3) {
					std::string str;
					str += "{";
					str += std::to_string(indices[i]) + ",";
					str += std::to_string(indices[i + 1]) + ",";
					str += std::to_string(indices[i + 2]) + "},\n";

					ostream << str.c_str();

				}
				ostream.close();
			}
		}
#endif





		//凸メッシュ作成

		desc.points.count = static_cast<PxU32>(vertices.size());
		desc.points.stride = sizeof(Vector3);
		desc.points.data = vertices.data();
		desc.indices.count = ref_poly_.PolygonNum;
		desc.indices.stride = sizeof(PxU32) * 3;
		desc.indices.data = indices.data();
		desc.flags |= PxConvexFlag::eCOMPUTE_CONVEX;
		desc.flags |= PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES;
		desc.flags |= PxConvexFlag::eFAST_INERTIA_COMPUTATION;

#ifndef PACKAGE_BUILD
		if (!desc.isValid())
			throw(Exception("ERROR!!DESC_INVALID", DEFAULT_EXCEPTION_PARAM));
#endif
		// CookingしてConvexMeshを作成
		PxDefaultMemoryOutputStream write_buffer;
		bool status{ PxCookConvexMesh(params, desc, write_buffer) };

#ifndef PACKAGE_BUILD
		if (!status)
			throw(Exception("ERROR!!STATUS_INVALID", DEFAULT_EXCEPTION_PARAM));

#endif

		PxDefaultMemoryInputData read_buffer(write_buffer.getData(), write_buffer.getSize());
		convex_mesh = PhysicsManager::GetPhysicsInstance()->createConvexMesh(read_buffer);
		Time::ResetTime();
#ifndef PACKAGE_BUILD
		if (!convex_mesh)
			throw(Exception("ERROR!!MESH_INVALID", DEFAULT_EXCEPTION_PARAM));
#endif

	}

	return convex_mesh;
}

physx::PxTriangleMesh* ModelSource::GetOrCreateTriangleMesh()
{
	if (!triangle_mesh) {
		physx::PxCookingParams params(PhysicsManager::GetPhysicsInstance()->getTolerancesScale());
		params.midphaseDesc = physx::PxMeshMidPhase::eBVH34;
		GetPolygon();
		physx::PxTriangleMeshDesc desc;
		auto meshroot_mat = MV1GetFrameBaseLocalMatrix(handle, 0);
		// 頂点データ取得
		std::vector<physx::PxVec3> vertices;
		for (int i = 0; i < ref_poly_.VertexNum; i++) {
			VECTOR vertex = VTransform(ref_poly_.Vertexs[i].Position, meshroot_mat);
			vertices.push_back(Vector3(vertex.x, vertex.y, vertex.z));
			ref_poly_.Vertexs[i].Position = vertex;
		}

		desc.points.count = static_cast<physx::PxU32>(vertices.size());
		desc.points.stride = sizeof(Vector3);
		desc.points.data = vertices.data();





		// インデックスデータ取得
		std::vector<physx::PxU32> indices;
		for (int i = 0; i < ref_poly_.PolygonNum; i++) {
			physx::PxU32 idx0 = ref_poly_.Polygons[i].VIndex[0];
			physx::PxU32 idx1 = ref_poly_.Polygons[i].VIndex[1];
			physx::PxU32 idx2 = ref_poly_.Polygons[i].VIndex[2];

			indices.push_back(idx0);
			indices.push_back(idx1);
			indices.push_back(idx2);
		}

		desc.triangles.count = ref_poly_.PolygonNum;
		desc.triangles.stride = sizeof(physx::PxU32) * 3;
		desc.triangles.data = indices.data();
		desc.flags = physx::PxMeshFlags();

#ifndef PACKAGE_BUILD
		if (!desc.isValid())
			throw(Exception("ERROR!!DESC_INVALID", DEFAULT_EXCEPTION_PARAM));

#endif

		physx::PxDefaultMemoryOutputStream write_buffer;
		bool status{ PxCookTriangleMesh(params,desc, write_buffer) };
#ifndef PACKAGE_BUILD
		if (!status)
			throw(Exception("ERROR!!STATUS_INVALID", DEFAULT_EXCEPTION_PARAM));
#endif

		physx::PxDefaultMemoryInputData read_buffer(write_buffer.getData(), write_buffer.getSize());
		triangle_mesh = PhysicsManager::GetPhysicsInstance()->createTriangleMesh(read_buffer);
		Time::ResetTime();
#ifndef PACKAGE_BUILD
		if (!triangle_mesh)
			throw(Exception("ERROR!!MESH_INVALID", DEFAULT_EXCEPTION_PARAM));
#endif



	}
	return triangle_mesh;
}

void Model::SetShader(Shader* pixel, Shader* vertex)
{
	shader_ps = pixel;
	shader_vs = vertex;
}

void Model::SetDefaultShader(Shader* pixel, Shader* vertex)
{
	default_shader_ps = pixel ? pixel : default_shader_ps;
	default_shader_vs = vertex ? vertex : default_shader_vs;
}

physx::PxConvexMesh* Model::GetConvexMesh()
{
	return original->GetOrCreateConvexMesh();
}

physx::PxTriangleMesh* Model::GetTriangleMesh()
{
	return original->GetOrCreateTriangleMesh();
}

Model::Model()
{
	instance++;
	default_shader_ps = nullptr;
	default_shader_vs = nullptr;
	shader_ps = nullptr;
	shader_vs = nullptr;
}

void Model::Draw()
{
	bool write_z = MV1GetOpacityRate(handle) < 1.0f;
	DxLib::SetWriteZBuffer3D(write_z);
	//フレーム(リグ)単位でメッシュを描画
	for (s32 frame_index = 0; frame_index < MV1GetFrameNum(handle); frame_index++)
	{
		//フレームのメッシュ数を取得
		s32 mesh_count = MV1GetFrameMeshNum(handle, frame_index);
		for (s32 mesh_index = 0; mesh_index < mesh_count; mesh_index++)
		{

			//メッシュ番号を取得
			s32 mesh = MV1GetFrameMesh(handle, frame_index, mesh_index);

			//メッシュのトライアングルリスト数を取得
			s32 tlist_cout = MV1GetMeshTListNum(handle, mesh);
			for (s32 tlist_index = 0; tlist_index < tlist_cout; tlist_index++)
			{
				//トライアングルリスト番号を取得
				s32 tlist = MV1GetMeshTList(handle, mesh, tlist_index);

				//トライアングルリストが使用しているマテリアルのインデックスを取得
				auto material_index = MV1GetTriangleListUseMaterial(handle, tlist);
				//--------------------------------------------------
				// シェーダーバリエーションを選択
				//--------------------------------------------------
				// 頂点データタイプ(DX_MV1_VERTEX_TYPE_1FRAME 等)
				auto vertex_type = MV1GetTriangleListVertexType(handle, tlist);
				u32  variant_vs = vertex_type;    // DXライブラリの頂点タイプをそのままバリエーション番号に

				//--------------------------------------------------
				// トライアングルリストを描画
				//--------------------------------------------------

				if (default_shader_ps && default_shader_vs) {

					int handle_vs = shader_vs ? shader_vs->variant(variant_vs) : default_shader_vs->variant(variant_vs);
					int handle_ps = shader_ps ? *shader_ps : *default_shader_ps;

					// シェーダーがない場合はオリジナルシェーダー利用を無効化
					bool enable_shader = (handle_vs != -1) && (handle_ps != -1);
					MV1SetUseOrigShader(enable_shader);
					SetUseVertexShader(handle_vs);
					SetUsePixelShader(handle_ps);
				}

				//トライアングルリストを描画
				MV1DrawTriangleList(handle, tlist);
			}

		}
	}
	MV1SetUseOrigShader(false);
	DxLib::SetWriteZBuffer3D(true);
}

void Animation::Update(float anim_timer)
{
	for (auto& call_back : call_backs) {
		if (!call_back.is_executed && anim_timer > call_back.ex_frame)
		{
			call_back.is_executed = true;
			if (call_back.function)
				call_back.function();
		}
	}
}

void Animation::InitCallBacks()
{
	for (auto& call_back : call_backs) {
		call_back.is_executed = false;
	}
}

void Animation::SetCallBack(const std::function<void()>& call_back, float execute_frame, std::string_view method_name)
{
	std::string name(method_name);
	auto it = method_names.find(name);
	if (it != method_names.end())
		return;
	AnimationCallBack anim_call_back;
	anim_call_back.function = std::move(call_back);
	anim_call_back.ex_frame = execute_frame;
	anim_call_back.is_executed = false;
	call_backs.push_back(std::move(anim_call_back));
	method_names[name] = call_backs.size();

}

void Animation::ResetCallBack(std::string_view method_name)
{
	std::string name(method_name);
	auto ite = method_names.find(name);
	if (ite == method_names.end())
		return;
	call_backs.erase(call_backs.begin() + ite->second - 1);
	method_names.erase(ite);
}
