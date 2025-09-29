#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <thread>

#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

namespace Audio {

// ��M���� RIFF �w�b�_ + PCM �����̂܂� Feed ����΍Đ������v���C���[
class WinMMWavePlayer {
public:
    WinMMWavePlayer();
    ~WinMMWavePlayer();

    // �l�b�g���[�N���Ŏ�M�����o�C�g������̂܂ܓn���i�C�ӕ���OK�j
    void Feed(const void* data, size_t size);

    // �Đ���~�E���\�[�X���
    void Stop();

    bool IsOpen() const noexcept { return opened_.load(std::memory_order_acquire); }

private:
    bool TryParseHeader();
    bool OpenFromParsedHeader();

    // waveOut �փo�b�t�@�������i�~���[�e�b�N�X��ێ��� waveOutWrite ���ĂԂ��߂̃��[�J�[���f���j
    void PumpThreadProc();

    static void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR /*dwParam2*/);

private:
    // ���͒~�ρi�w�b�_�{PCM�j
    std::vector<uint8_t> inputBuffer_;
    size_t headerNeeded_ = 44; // �ŏ� RIFF �w�b�_

    // ��͍ς݃w�b�_
    WAVEFORMATEX wfx_{};
    bool headerParsed_ = false;

    // waveOut �Đ��n
    HWAVEOUT hWaveOut_ = nullptr;
    std::atomic<bool> opened_{false};

    // �o�̓o�b�t�@
    static constexpr int BUFFER_COUNT = 4;
    std::vector<uint8_t> buffers_[BUFFER_COUNT];
    WAVEHDR headers_[BUFFER_COUNT]{};
    uint32_t bufferBytes_ = 0;

    // ����
    std::mutex mtx_;
    std::condition_variable cv_;
    std::thread pumpThread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> needPump_{false};
};

} // namespace Audio