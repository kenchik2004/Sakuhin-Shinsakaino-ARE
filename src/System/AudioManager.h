#pragma once

class AudioSource {
	friend class AudioManager;
	std::string path;
	std::string name;
	int handle = -1;
	bool is_loaded = false;
public:
	~AudioSource() {
		if (handle >= 0) {
			DeleteSoundMem(handle);
		}
	}
};


class AudioClip {
	friend class AudioManager;
	friend class AudioPlayer;
	std::string name;
	int handle = -1;
	static inline int instance = 0;
public:
	AudioClip() { instance++; }
	AudioClip(const AudioClip& other) {
		name = other.name;
		handle = other.handle;
		instance++;
	}
	~AudioClip() {
		StopSoundMem(handle);
		DeleteSoundMem(handle);
		instance--;
	}
	void PlayOneShot();
};




class AudioManager
{
	AudioManager() = delete;
	AudioManager(const AudioManager&) = delete;
	~AudioManager() = default;
public:
	static inline int loading_count = 0;
	struct IndexAndHandle {
		int index = -1;
		int handle = -1;
	};
	struct PtrToCacheAndAudioData {
		std::vector<SafeUniquePtr<AudioSource>>* cache;
		std::unordered_map<std::string, IndexAndHandle>* name_map;
		std::unordered_map<std::string, IndexAndHandle>* path_map;
		SafeUniquePtr<AudioSource> audio_source;
	};
	static std::vector<SafeUniquePtr<AudioSource>> cache;
	static std::unordered_map<std::string, IndexAndHandle> names;
	static std::unordered_map<std::string, IndexAndHandle> paths;
	static void Load(std::string_view path, std::string_view name, bool use_3d = false);
	static SafeSharedPtr<AudioClip> CloneByName(std::string_view name, std::string_view new_name = "");
	static SafeSharedPtr<AudioClip> CloneByPath(std::string_view path, std::string_view new_name = "");
	static inline const int GetLoadingCount() { return loading_count; }
	static void Init();
	static void Exit();
};

