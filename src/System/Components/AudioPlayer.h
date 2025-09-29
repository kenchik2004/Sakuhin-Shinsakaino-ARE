#pragma once
#include "System/Component.h"

USING_PTR(AudioPlayer);
class AudioPlayer :
	public Component
{
public:
	USING_SUPER(AudioPlayer);
	void Construct() override;
	SafeSharedPtr<AudioClip> audio = nullptr;
	void SetAudio(std::string_view name, std::string_view new_name = "");
	void SetAudio(SafeSharedPtr<AudioClip> audio_);

	inline SafeSharedPtr<AudioClip> GetAudio() {
		return audio;
	}

	void Play(float start_pos = 0, int sample_rate = -1);
	void PreDraw() override;
	void Stop();
	static void Load(std::string_view path, std::string_view name, bool use_3d = true);
	void Exit() override;
	int default_frequency;
	float pitch_rate = 1.0f;
	float radius = 200;
	bool loop = false;
};

