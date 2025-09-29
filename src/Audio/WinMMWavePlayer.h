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

// 受信側で RIFF ヘッダ + PCM をそのまま Feed すれば再生されるプレイヤー
class WinMMWavePlayer {
public:
    WinMMWavePlayer();
    ~WinMMWavePlayer();

    // ネットワーク等で受信したバイト列をそのまま渡す（任意分割OK）
    void Feed(const void* data, size_t size);

    // 再生停止・リソース解放
    void Stop();

    bool IsOpen() const noexcept { return opened_.load(std::memory_order_acquire); }

private:
    bool TryParseHeader();
    bool OpenFromParsedHeader();

    // waveOut へバッファを供給（ミューテックス非保持で waveOutWrite を呼ぶためのワーカーモデル）
    void PumpThreadProc();

    static void CALLBACK WaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR /*dwParam2*/);

private:
    // 入力蓄積（ヘッダ＋PCM）
    std::vector<uint8_t> inputBuffer_;
    size_t headerNeeded_ = 44; // 最小 RIFF ヘッダ

    // 解析済みヘッダ
    WAVEFORMATEX wfx_{};
    bool headerParsed_ = false;

    // waveOut 再生系
    HWAVEOUT hWaveOut_ = nullptr;
    std::atomic<bool> opened_{false};

    // 出力バッファ
    static constexpr int BUFFER_COUNT = 4;
    std::vector<uint8_t> buffers_[BUFFER_COUNT];
    WAVEHDR headers_[BUFFER_COUNT]{};
    uint32_t bufferBytes_ = 0;

    // 同期
    std::mutex mtx_;
    std::condition_variable cv_;
    std::thread pumpThread_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> needPump_{false};
};

} // namespace Audio