#include "precompile.h"
#include "AudioListener.h"




void AudioListener::Construct()
{
	status.status_bit.on(CompStat::STATUS::SINGLE);
}

int AudioListener::Init()
{
	if (!GetCurrentListener())
		SetCurrentListener();
	return 0;
}

void AudioListener::PreDraw()
{
	if (is_current_listener)
		Set3DSoundListenerPosAndFrontPosAndUpVec(cast(owner->transform->position), cast(owner->transform->position + owner->transform->AxisZ()), cast(owner->transform->AxisY()));

}

void AudioListener::SetCurrentListener()
{
	auto current_listener = owner->GetScene()->GetCurrentAudioListener();
	if (current_listener)
		SafeStaticCast<AudioListener>(current_listener.lock())->is_current_listener = false;
	is_current_listener = true;
	owner->GetScene()->SetCurrentAudioListener(std::static_pointer_cast<AudioListener>(shared_from_this()));
}

SafeSharedPtr<AudioListener> AudioListener::GetCurrentListener()
{
	return SafeStaticCast<AudioListener>(owner->GetScene()->GetCurrentAudioListener().lock());
}
