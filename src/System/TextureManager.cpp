//---------------------------------------------------------------------------
//! @file   TextureManager.cpp
//! @brief  テクスチャ管理
//---------------------------------------------------------------------------
#include "precompile.h"
#include "TextureManager.h"
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

std::vector<SafeUniquePtr<TextureSource>> TextureManager::cache = std::vector<SafeUniquePtr<TextureSource>>(0);
std::unordered_map<std::string, TextureManager::IndexAndHandle> TextureManager::names = std::unordered_map<std::string, TextureManager::IndexAndHandle>(0);
std::unordered_map<std::string, TextureManager::IndexAndHandle> TextureManager::paths = std::unordered_map<std::string, TextureManager::IndexAndHandle>(0);

//! @brief  ロード処理
//! @param  std::string_view テクスチャのパス
//! @param  std::string_view テクスチャにつける名前
//! @details	与えられたパスを基にテクスチャを非同期でロード・キャッシュする
void TextureManager::Load(std::string_view path, std::string_view name)
{
	std::string path_key(path);
	std::string name_key(name);
	if (paths.count(path_key) || names.count(name_key))
		return;
	auto texture = make_safe_unique<TextureSource>();
	texture->name = name_key;
	texture->path = path_key;
	auto call_back = [](int handle, void* data) {
		auto ptr = reinterpret_cast<PtrToCacheAndTextureData*>(data);
		ptr->texture_source->is_loaded = true;
		int cache_index = ptr->cache->size();
		auto name = ptr->texture_source->name;
		auto path = ptr->texture_source->path;
		ptr->cache->push_back(std::move(ptr->texture_source));
		(*(ptr->name_map))[name].index = cache_index;
		(*(ptr->path_map))[path].index = cache_index;
		loading_count--;
		delete ptr;
		};
	PtrToCacheAndTextureData* data = new PtrToCacheAndTextureData;
	data->cache = &cache;
	data->name_map = &names;
	data->path_map = &paths;
	data->texture_source = std::move(texture);
	SetUseASyncLoadFlag(true);
	data->texture_source->handle = LoadGraph(path_key.c_str());
	names[name_key].handle = data->texture_source->handle;
	paths[path_key].handle = data->texture_source->handle;
	SetASyncLoadFinishCallback(data->texture_source->handle, call_back, data);
	loading_count++;
	SetUseASyncLoadFlag(false);
	return;
}

SafeSharedPtr<Texture> TextureManager::Create(std::string_view name, int width, int height)
{
	std::string name_key(name);
	if (names.count(name_key))
		return nullptr;
	auto texture = make_safe_unique<TextureSource>();
	texture->name = name_key;
	texture->handle = MakeScreen(width, height, true);
	names[name_key].handle = texture->handle;
	paths[name_key].handle = texture->handle;
	int cache_index = cache.size();
	names[name_key].index = cache_index;
	paths[name_key].index = cache_index;
	texture->is_loaded = true;
	auto texture_ = make_safe_shared<Texture>();
	texture_->handle = texture->handle;
	texture_->name = name_key;
	cache.push_back(std::move(texture));
	return texture_;

}

SafeSharedPtr<Texture> TextureManager::CloneByName(std::string_view name, std::string_view new_name)
{

	// ここで描画処理など
	std::string name_(name);
	SafeSharedPtr<Texture> texture = nullptr;
	auto resource = names.find(name_);
	if (resource != names.end() && resource->second.handle >= 0) {
		texture = make_safe_shared<Texture>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		while (resource->second.index < 0) {}
		int x = 0, y = 0;
		GetGraphSize(cache[resource->second.index]->handle, &x, &y);

		void* tex2Dfrom = const_cast<void*>(GetGraphID3D11Texture2D(cache[resource->second.index]->handle));
		if (tex2Dfrom) {
			auto tex2Dfrom_ = reinterpret_cast<ID3D11Texture2D*>(tex2Dfrom);
			D3D11_TEXTURE2D_DESC descFrom = {};
			tex2Dfrom_->GetDesc(&descFrom);
			if (descFrom.BindFlags & D3D11_BIND_RENDER_TARGET) {
				// もし元画像がRenderTargetとして作成されたなら、MakeScreenで作成
				texture->handle = MakeScreen(x, y, true);
			}
			else {
				// それ以外ならMakeGraphで作成
				texture->handle = MakeGraph(x, y);
			}
		}
		void* tex2Dto = const_cast<void*>(GetGraphID3D11Texture2D(texture->handle));
		void* context = const_cast<void*>(GetUseDirect3D11DeviceContext());
		// もしtex2Dfromとtex2Dtoが両方とも存在し、contextも有効ならコピーする
		if (tex2Dfrom && tex2Dto && context) {
			auto tex2Dfrom_ = reinterpret_cast<ID3D11Texture2D*>(tex2Dfrom);
			auto tex2Dto_ = reinterpret_cast<ID3D11Texture2D*>(tex2Dto);
			auto context_ = reinterpret_cast<ID3D11DeviceContext*>(context);
			D3D11_TEXTURE2D_DESC descFrom = {};
			tex2Dfrom_->GetDesc(&descFrom);
			D3D11_TEXTURE2D_DESC descTo = {};
			tex2Dto_->GetDesc(&descTo);
			// もし両方ともD3D11_USAGE_DEFAULTならCopyResourceを使う
			if (descTo.Usage == D3D11_USAGE_DEFAULT && descFrom.Usage == D3D11_USAGE_DEFAULT) {
				context_->CopyResource(tex2Dto_, tex2Dfrom_);
			}
		}
		//もしコピーができないなら、無理にコピーしない
		//空っぽのままおいておく
		texture->name = new_name == "" ? cache[resource->second.index]->name : new_name;

	}                   
	return texture;
}

SafeSharedPtr<Texture> TextureManager::CloneByPath(std::string_view path, std::string_view new_name)
{
	std::string path_(path);
	SafeSharedPtr<Texture> texture = nullptr;
	auto resource = paths.find(path_);
	if (resource != paths.end() && resource->second.handle >= 0) {
		texture = make_safe_shared<Texture>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		while (resource->second.index < 0) {}
		int x = 0, y = 0;
		GetGraphSize(cache[resource->second.index]->handle, &y, &y);
		texture->handle = DerivationGraph(0, 0, x, y, cache[resource->second.index]->handle);
		texture->name = new_name == "" ? cache[resource->second.index]->name : new_name;

	}
	return texture;
}

void TextureManager::Init()
{
}

void TextureManager::Exit()
{
	WaitHandleASyncLoadAll();
	cache.clear();
	names.clear();
	paths.clear();
}
