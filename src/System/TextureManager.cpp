//---------------------------------------------------------------------------
//! @file   TextureManager.cpp
//! @brief  テクスチャ管理
//---------------------------------------------------------------------------
#include "precompile.h"
#include "TextureManager.h"
#include <d3d11.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")

std::vector<SafeUniquePtr<TextureSource>> TextureManager::cache = std::vector<SafeUniquePtr<TextureSource>>(0);
std::unordered_map<std::string, TextureManager::IndexAndHandle> TextureManager::names = std::unordered_map<std::string, TextureManager::IndexAndHandle>(0);
std::unordered_map<std::string, TextureManager::IndexAndHandle> TextureManager::paths = std::unordered_map<std::string, TextureManager::IndexAndHandle>(0);
ID3D11Device* TextureManager::s_device = nullptr;
ID3D11DeviceContext* TextureManager::s_context = nullptr;

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

		// 幅・高さを取得
		int w = 0, h = 0;
		GetGraphSize(handle, &w, &h);
		ptr->texture_source->width = static_cast<u32>(w);
		ptr->texture_source->height = static_cast<u32>(h);

		// DxLib から D3D11 リソース/ビューを取得（無ければ一部は自前生成）
		ID3D11Device* device = reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
		ID3D11DeviceContext* context = reinterpret_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));
		// Texture2D
		if (void* p = const_cast<void*>(GetGraphID3D11Texture2D(handle))) {
			//Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d = reinterpret_cast<ID3D11Texture2D*>(p);
			//Microsoft::WRL::ComPtr<ID3D11Resource> res;
			//tex2d.As(&res);
			//
			auto tex2d = reinterpret_cast<ID3D11Texture2D*>(p);
			ptr->texture_source->handle = handle;
			ptr->texture_source->Init(tex2d);

		}

		int cache_index = static_cast<int>(ptr->cache->size());
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

SafeSharedPtr<Texture> TextureManager::Create(std::string_view name, int width, int height, DXGI_FORMAT format)
{

	auto tex = make_safe_shared<Texture>(width, height, format);
	if (tex)
		tex->name = name.empty() ? "CREATED:UNTITLED" : std::string(name);
	return tex;
}

SafeSharedPtr<Texture> TextureManager::CloneByName(std::string_view name, std::string_view new_name)
{

	std::string name_(name);
	SafeSharedPtr<Texture> texture = nullptr;
	auto resource = names.find(name_);
	if (resource != names.end() && resource->second.handle >= 0) {
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		while (resource->second.index < 0) {}
		auto& cache_tex = *(cache[resource->second.index]);
		texture = make_safe_shared<Texture>(cache_tex);
		if (!new_name.empty())
			texture->name = new_name;
		// キャッシュからクローンを作成するので、D3Dリソースはコピーする
		s_context->CopyResource(texture->texture.Get(), cache_tex.texture.Get());
	}
	return texture;
}

SafeSharedPtr<Texture> TextureManager::CloneByPath(std::string_view path, std::string_view new_name)
{
	std::string path_(path);
	SafeSharedPtr<Texture> texture = nullptr;
	auto resource = paths.find(path_);
	if (resource != paths.end() && resource->second.handle >= 0) {
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		while (resource->second.index < 0) {}
		auto& cache_tex = *(cache[resource->second.index]);
		texture = make_safe_shared<Texture>(cache_tex);
		if (!new_name.empty())
			texture->name = new_name;

	}
	return texture;
}

void TextureManager::Init()
{
	// Direct3D11 デバイスとコンテキストを取得
	s_device = reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
	s_context = reinterpret_cast<ID3D11DeviceContext*>(const_cast<void*>(GetUseDirect3D11DeviceContext()));

}

void TextureManager::Exit()
{
	WaitHandleASyncLoadAll();
	cache.clear();
	names.clear();
	paths.clear();

	s_device = nullptr;
	s_context = nullptr;
}

Texture::Texture(const TextureSource& source)
{
	D3D11_TEXTURE2D_DESC desc;
	reinterpret_cast<ID3D11Texture2D*>(source.texture.Get())->GetDesc(&desc);
	ID3D11Device* device = TextureManager::Device();
	if (device) {
		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d;
		device->CreateTexture2D(&desc, nullptr, tex2d.GetAddressOf());

		is_initialized = Init(tex2d.Get());

		// DxLib のハンドルも作成
		if (handle < 0)
			handle = DxLib::CreateGraphFromID3D11Texture2D(texture.Get());
	}
	if (is_initialized) {
		name = "CLONED:" + source.name;
	}
}
void TextureSource::Init(ID3D11Resource* d3d_resource)
{
	if (!d3d_resource)
		return;
	D3D11_RESOURCE_DIMENSION dimension;
	d3d_resource->GetType(&dimension);

	if (dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
		return;
	}

	texture = d3d_resource;
	auto* d3d_device = reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
	auto* d3d_texture_2d = reinterpret_cast<ID3D11Texture2D*>(d3d_resource);

	D3D11_TEXTURE2D_DESC desc;
	d3d_texture_2d->GetDesc(&desc);

	width = desc.Width;     // 幅
	height = desc.Height;    // 高さ

	if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
		if (desc.Format == DXGI_FORMAT_R32_TYPELESS) {    // デプスバッファ用の場合はR32_FLOATとして利用
			D3D11_SHADER_RESOURCE_VIEW_DESC view_desc{};
			view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			view_desc.Format = DXGI_FORMAT_R32_FLOAT;
			view_desc.Texture2D.MipLevels = 1;
			d3d_device->CreateShaderResourceView(d3d_resource, &view_desc, &srv);
		}
		else {
			d3d_device->CreateShaderResourceView(d3d_resource, nullptr, &srv);

			// ID3D11Texture2DからDxLibグラフィックハンドルを作成
			if (handle < 0) {
				handle = CreateGraphFromID3D11Texture2D(d3d_resource);
			}
		}
	}

	if (desc.BindFlags & D3D11_BIND_RENDER_TARGET) {
		d3d_device->CreateRenderTargetView(d3d_resource, nullptr, &rtv);
	}

	if (desc.BindFlags & D3D11_BIND_DEPTH_STENCIL) {
		D3D11_DEPTH_STENCIL_VIEW_DESC view_desc{};
		view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		if (desc.Format == DXGI_FORMAT_R16_TYPELESS)
			view_desc.Format = DXGI_FORMAT_D16_UNORM;

		else if (desc.Format == DXGI_FORMAT_D32_FLOAT || desc.Format == DXGI_FORMAT_R32_TYPELESS)
			view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		else
			view_desc.Format = DXGI_FORMAT_UNKNOWN;

		d3d_device->CreateDepthStencilView(d3d_resource, &view_desc, &dsv);
	}

}

bool Texture::Init(ID3D11Resource* d3d_resource)
{
	if (!d3d_resource)
		return false;
	D3D11_RESOURCE_DIMENSION dimension;
	d3d_resource->GetType(&dimension);

	if (dimension != D3D11_RESOURCE_DIMENSION_TEXTURE2D) {
		return false;
	}

	texture = d3d_resource;
	auto* d3d_device = reinterpret_cast<ID3D11Device*>(const_cast<void*>(GetUseDirect3D11Device()));
	auto* d3d_texture_2d = reinterpret_cast<ID3D11Texture2D*>(d3d_resource);

	D3D11_TEXTURE2D_DESC desc;
	d3d_texture_2d->GetDesc(&desc);

	width = desc.Width;     // 幅
	height = desc.Height;    // 高さ

	if (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
		if (desc.Format == DXGI_FORMAT_R32_TYPELESS) {    // デプスバッファ用の場合はR32_FLOATとして利用
			D3D11_SHADER_RESOURCE_VIEW_DESC view_desc{};
			view_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
			view_desc.Format = DXGI_FORMAT_R32_FLOAT;
			view_desc.Texture2D.MipLevels = 1;
			d3d_device->CreateShaderResourceView(d3d_resource, &view_desc, &srv);
		}
		else {
			d3d_device->CreateShaderResourceView(d3d_resource, nullptr, &srv);

			// ID3D11Texture2DからDxLibグラフィックハンドルを作成
			if (handle < 0) {
				handle = CreateGraphFromID3D11Texture2D(d3d_resource);
			}
		}
	}

	if (desc.BindFlags & D3D11_BIND_RENDER_TARGET) {
		d3d_device->CreateRenderTargetView(d3d_resource, nullptr, &rtv);
	}

	if (desc.BindFlags & D3D11_BIND_DEPTH_STENCIL) {
		D3D11_DEPTH_STENCIL_VIEW_DESC view_desc{};
		view_desc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		if (desc.Format == DXGI_FORMAT_R16_TYPELESS || desc.Format == DXGI_FORMAT_R16_UNORM)
			view_desc.Format = DXGI_FORMAT_D16_UNORM;

		else if (desc.Format == DXGI_FORMAT_D32_FLOAT || desc.Format == DXGI_FORMAT_R32_TYPELESS)
			view_desc.Format = DXGI_FORMAT_D32_FLOAT;
		else
			view_desc.Format = DXGI_FORMAT_UNKNOWN;

		d3d_device->CreateDepthStencilView(d3d_resource, &view_desc, &dsv);
	}

	return true;
}

Texture::Texture(ID3D11Resource* d3d_resource)
{
	is_initialized = Init(d3d_resource);
}

Texture::Texture(int dxlib_handle)
{
	DxLib::GetGraphSize(handle, reinterpret_cast<int*>(&width), reinterpret_cast<int*>(&height));

	ID3D11Resource* d3d_resource = reinterpret_cast<ID3D11Resource*>(const_cast<void*>(GetGraphID3D11Texture2D(handle)));
	texture = d3d_resource;
	rtv = reinterpret_cast<ID3D11RenderTargetView*>(const_cast<void*>(GetGraphID3D11RenderTargetView(handle)));
	dsv = reinterpret_cast<ID3D11DepthStencilView*>(const_cast<void*>(GetGraphID3D11DepthStencilView(handle)));
	handle = dxlib_handle;

	is_initialized = true;

}

Texture::Texture(u32 width_, u32 height_, DXGI_FORMAT format)
{
	width = width_;
	height = height_;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = static_cast<UINT>(width_);
	desc.Height = static_cast<UINT>(height_);
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = format;
	desc.SampleDesc = { 1,0 };
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;

	bool is_depth_stencil = format == DXGI_FORMAT_D24_UNORM_S8_UINT || format == DXGI_FORMAT_D32_FLOAT;
	if (is_depth_stencil)
	{
		// 深度ステンシルなら DSV も作成
		// フォーマットが D24S8 なら R24G8_TYPELESS、D32 なら R32_TYPELESS にする
		desc.Format = (format == DXGI_FORMAT_D24_UNORM_S8_UINT) ? DXGI_FORMAT_R24G8_TYPELESS : DXGI_FORMAT_R32_TYPELESS;
		desc.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
	}
	else
	{
		desc.BindFlags |= D3D11_BIND_RENDER_TARGET;
	}

	ID3D11Device* device = TextureManager::Device();
	if (device) {
		Microsoft::WRL::ComPtr<ID3D11Texture2D> tex2d;
		device->CreateTexture2D(&desc, nullptr, tex2d.GetAddressOf());

		Init(tex2d.Get());

		// DxLib のハンドルも作成
		if (handle < 0 && !is_depth_stencil)
			handle = DxLib::CreateGraphFromID3D11Texture2D(texture.Get());
		is_initialized = true;
	}

}

Texture::operator int()
{
	if (!is_initialized)
		return DX_NONE_GRAPH;
	return handle >= 0 ? handle : DX_NONE_GRAPH;
}
