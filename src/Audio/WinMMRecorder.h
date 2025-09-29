#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <atomic>

#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace Audio {

class WinMMRecorder {
public:
    // isHeader == true �̂Ƃ��� RIFF/WAVE �w�b�_ (��x����)
    // isHeader == false �̂Ƃ��� �A������ PCM �f�[�^
    using WaveChunkCallback = std::function<void(const uint8_t* data, size_t size, bool isHeader)>;

    // bufferMillis: 1 �o�b�t�@�̒���[�~���b]�B��d�o�b�t�@�ŉ^�p����܂��B
    explicit WinMMRecorder(
        uint32_t sampleRate    = 48000,
        uint16_t bitsPerSample = 16,
        uint16_t channels      = 1,
        uint32_t bufferMillis  = 50);

    ~WinMMRecorder();

    // �^���J�n/��~
    bool Start();
    void Stop();

    bool IsRunning() const noexcept { return running_.load(std::memory_order_acquire); }

    // Wave �f�[�^�r�o�R�[���o�b�N��ݒ�
    void SetOnWaveChunk(WaveChunkCallback cb) { callback_ = std::move(cb); }

private:
    static void CALLBACK WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR /*dwParam2*/);

    void EmitWaveHeaderIfNeeded();
    std::vector<uint8_t> BuildWaveHeader(uint32_t dataBytesPlaceholder) const;
    bool QueueBuffer(size_t index);

    HWAVEIN       hWaveIn_ = nullptr;
    WAVEFORMATEX  wfx_{};

    std::vector<uint8_t> buffers_[2];
    WAVEHDR       headers_[2]{};

    
    uint32_t      bufferMillis_ = 0;

    std::atomic<bool> running_{false};
    std::atomic<bool> stopping_{false};
    std::atomic<bool> headerEmitted_{false};

    WaveChunkCallback callback_;
};

} // namespace Audio