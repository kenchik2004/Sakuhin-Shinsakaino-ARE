#include "precompile.h"
#include "AudioManager.h"



std::vector<SafeUniquePtr<AudioSource>> AudioManager::cache;
std::unordered_map<std::string, AudioManager::IndexAndHandle> AudioManager::names;
std::unordered_map<std::string, AudioManager::IndexAndHandle> AudioManager::paths;
void AudioManager::Load(std::string_view path, std::string_view name, bool use_3d)
{
	std::string path_key(path);
	std::string name_key(name);
	if (paths.count(path_key) || names.count(name_key))
		return;
	auto audio = make_safe_unique<AudioSource>();
	audio->name = name_key;
	audio->path = path_key;
	auto call_back = [](int handle, void* data) {
		auto ptr = reinterpret_cast<PtrToCacheAndAudioData*>(data);
		ptr->audio_source->is_loaded = true;
		size_t cache_index = ptr->cache->size();
		auto name = ptr->audio_source->name;
		auto path = ptr->audio_source->path;
		(*(ptr->name_map))[name].index = cache_index;
		(*(ptr->path_map))[path].index = cache_index;
		ptr->cache->push_back(std::move(ptr->audio_source));
		loading_count--;
		delete ptr;
		};
	PtrToCacheAndAudioData* data = new PtrToCacheAndAudioData;
	data->cache = &cache;
	data->name_map = &names;
	data->path_map = &paths;
	data->audio_source = std::move(audio);
	SetUseASyncLoadFlag(true);
	SetCreate3DSoundFlag(use_3d);
	data->audio_source->handle = LoadSoundMem(path_key.c_str());
	names[name_key].handle = data->audio_source->handle;
	paths[path_key].handle = data->audio_source->handle;
	SetASyncLoadFinishCallback(data->audio_source->handle, call_back, data);
	loading_count++;
	SetUseASyncLoadFlag(false);
	return;
}

SafeSharedPtr<AudioClip> AudioManager::CloneByName(std::string_view name, std::string_view new_name) {
	std::string name_(name);
	SafeSharedPtr<AudioClip> audio = nullptr;
	auto resource = names.find(name_);
	if (resource != names.end() && resource->second.handle >= 0) {
		audio = make_safe_shared<AudioClip>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		audio->handle = DuplicateSoundMem(cache[resource->second.index]->handle);
		//DXLibは3分以上の音声をクローンできないので、しゃあなしでオリジナルを渡す。
		//オリジナルの方は、非同期で再ロードし、できるだけノータイムで次のクローンができるようにする。
		if (audio->handle < 0) {
			audio->handle = cache[resource->second.index]->handle;
			//オリジナルがデストラクタでhandleを消してしまうので、ハンドルを無効にする
			cache[resource->second.index]->handle = -1;
			std::string path = cache[resource->second.index]->path;
			//オリジナルはいなかったことにして、再ロードする
			names.erase(name_);
			paths.erase(path);
			Load(path, name_);
			audio->name = new_name == "" ? name : new_name;
			return audio;
		}
		audio->name = new_name == "" ? cache[resource->second.index]->name : new_name;
	}
	return audio;
}

SafeSharedPtr<AudioClip> AudioManager::CloneByPath(std::string_view path, std::string_view new_name)
{
	std::string path_(path);
	SafeSharedPtr<AudioClip> audio = nullptr;
	auto resource = paths.find(path_);
	if (resource != paths.end() && resource->second.handle >= 0) {
		audio = make_safe_shared<AudioClip>();
		if (resource->second.index < 0)
			WaitHandleASyncLoad(resource->second.handle);
		audio->handle = DuplicateSoundMem(cache[resource->second.index]->handle);
		//DXLibは3分以上の音声をクローンできないので、しゃあなしでオリジナルを渡す。
		//オリジナルの方は、非同期で再ロードし、できるだけノータイムで次のクローンができるようにする。
		if (audio->handle < 0) {
			audio->handle = cache[resource->second.index]->handle;
			//オリジナルがデストラクタでhandleを消してしまうので、ハンドルを無効にする
			cache[resource->second.index]->handle = -1;
			std::string name = cache[resource->second.index]->name;
			//オリジナルはいなかったことにして、再ロードする
			names.erase(name);
			paths.erase(path_);
			Load(path, name);
			audio->name = new_name == "" ? name : new_name;
			return audio;
		}
		audio->name = new_name == "" ? cache[resource->second.index]->name : new_name;
	}
	return audio;
}

void AudioManager::Init()
{
}

void AudioManager::Exit()
{
	WaitHandleASyncLoadAll();
	cache.clear();
	names.clear();
	paths.clear();
	if (AudioClip::instance > 0) {
		std::string msg = typeid(AudioClip).name();
#ifndef PACKAGE_BUILD
		throw(MemoryLeakException(msg.c_str(), DEFAULT_EXCEPTION_PARAM));
#endif
	}
}

void AudioClip::PlayOneShot() {
	ChangeVolumeSoundMem(255, handle);
	PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}
