#include "WinMMWavePlayer.h"
#include <cstring>
#include <algorithm>

namespace Audio {

WinMMWavePlayer::WinMMWavePlayer() {}

WinMMWavePlayer::~WinMMWavePlayer() {
    Stop();
}

void WinMMWavePlayer::Stop() {
    // まずポンプスレッドを停止・合流
    stop_.store(true, std::memory_order_release);
    cv_.notify_all();
    if (pumpThread_.joinable()) {
        pumpThread_.join();
    }

    std::lock_guard<std::mutex> lk(mtx_);

    if (hWaveOut_) {
        waveOutReset(hWaveOut_);
        for (int i = 0; i < BUFFER_COUNT; ++i) {
            if (headers_[i].dwFlags & WHDR_PREPARED) {
                waveOutUnprepareHeader(hWaveOut_, &headers_[i], sizeof(WAVEHDR));
            }
        }
        waveOutClose(hWaveOut_);
        hWaveOut_ = nullptr;
    }

    opened_.store(false, std::memory_order_release);
    headerParsed_ = false;
    headerNeeded_ = 44;
    inputBuffer_.clear();
    for (int i = 0; i < BUFFER_COUNT; ++i) {
        buffers_[i].clear();
        std::memset(&headers_[i], 0, sizeof(WAVEHDR));
    }
    std::memset(&wfx_, 0, sizeof(wfx_));

    needPump_.store(false, std::memory_order_release);
    stop_.store(false, std::memory_order_release);
}

void WinMMWavePlayer::Feed(const void* data, size_t size) {
    if (!data || size == 0) return;
    {
        std::lock_guard<std::mutex> lk(mtx_);

        // データを蓄積
        const size_t oldSize = inputBuffer_.size();
        inputBuffer_.resize(oldSize + size);
        std::memcpy(inputBuffer_.data() + oldSize, data, size);

        // ヘッダ未解析なら試みる
        if (!headerParsed_) {
            if (!TryParseHeader()) {
                return; // ヘッダがまだ足りない、または未検出
            }
            // ヘッダからデバイスを開く
            if (!OpenFromParsedHeader()) {
                // 失敗時は止める
                // ロックを解いてから Stop したいのでフラグだけ立てる
            }
        }

        // 供給要求
        needPump_.store(true, std::memory_order_release);
    }
    cv_.notify_one();
}

static inline uint16_t read_le_u16(const uint8_t* p) {
    uint16_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}
static inline uint32_t read_le_u32(const uint8_t* p) {
    uint32_t v;
    std::memcpy(&v, p, sizeof(v));
    return v;
}

bool WinMMWavePlayer::TryParseHeader() {
    auto& buf = inputBuffer_;
    if (buf.size() < 12) {
        return false;
    }

    // 先頭から "RIFF" / "WAVE" をスキャン（先頭ノイズを破棄）
    size_t start = SIZE_MAX;
    for (size_t i = 0; i + 12 <= buf.size(); ++i) {
        if (std::memcmp(buf.data() + i, "RIFF", 4) == 0 &&
            std::memcmp(buf.data() + i + 8, "WAVE", 4) == 0) {
            start = i;
            break;
        }
    }
    if (start == SIZE_MAX) {
        if (buf.size() > 11) {
            buf.erase(buf.begin(), buf.begin() + (buf.size() - 11));
        }
        return false;
    }

    if (start > 0) {
        buf.erase(buf.begin(), buf.begin() + start);
    }
    if (buf.size() < 12) return false; // "RIFF" + size + "WAVE"

    // チャンク列を走査して fmt/data を見つける
    size_t offset = 12; // "RIFFxxxxWAVE" の直後
    bool haveFmt = false;
    WAVEFORMATEX tmp{};

    while (true) {
        if (buf.size() < offset + 8) return false;

        const uint8_t* ch = buf.data() + offset;
        char id[5] = {0};
        std::memcpy(id, ch, 4);
        const uint32_t size = read_le_u32(ch + 4);

        // 本体未到達なら待機（dataチャンクは本体不要）
        if (std::memcmp(id, "data", 4) != 0) {
            if (buf.size() < offset + 8 + size) return false;
        }

        if (std::memcmp(id, "fmt ", 4) == 0) {
            if (size < 16) return false;
            const uint8_t* p = ch + 8;
            const uint16_t audioFormat  = read_le_u16(p + 0);
            const uint16_t channels     = read_le_u16(p + 2);
            const uint32_t sampleRate   = read_le_u32(p + 4);
            const uint32_t byteRate     = read_le_u32(p + 8);
            const uint16_t blockAlign   = read_le_u16(p + 12);
            const uint16_t bitsPerSample= read_le_u16(p + 14);
            if (audioFormat != WAVE_FORMAT_PCM) return false;

            std::memset(&tmp, 0, sizeof(tmp));
            tmp.wFormatTag = WAVE_FORMAT_PCM;
            tmp.nChannels = channels;
            tmp.nSamplesPerSec = sampleRate;
            tmp.nAvgBytesPerSec = byteRate;
            tmp.nBlockAlign = blockAlign;
            tmp.wBitsPerSample = bitsPerSample;

            offset += 8 + size + (size & 1);
        }
        else if (std::memcmp(id, "data", 4) == 0) {
            if (!tmp.nBlockAlign) return false; // fmt 未解析
            const size_t dataOffset = offset + 8;
            if (buf.size() < dataOffset) return false;

            // ヘッダ部を削除して PCM のみ残す
            buf.erase(buf.begin(), buf.begin() + dataOffset);
            wfx_ = tmp;
            headerParsed_ = true;
            return true;
        }
        else {
            // LIST, JUNK, fact 等スキップ（2バイト境界）
            offset += 8 + size + (size & 1);
            if (offset > buf.size()) return false;
        }
    }
}

bool WinMMWavePlayer::OpenFromParsedHeader() {
    if (opened_.load(std::memory_order_acquire)) return true;

    const uint32_t bytesPer50ms = std::max<uint32_t>(wfx_.nAvgBytesPerSec / 20, wfx_.nBlockAlign);
    bufferBytes_ = (bytesPer50ms / wfx_.nBlockAlign) * wfx_.nBlockAlign;
    if (bufferBytes_ == 0) bufferBytes_ = wfx_.nBlockAlign;

    MMRESULT mm = waveOutOpen(
        &hWaveOut_,
        WAVE_MAPPER,
        &wfx_,
        reinterpret_cast<DWORD_PTR>(&WinMMWavePlayer::WaveOutProc),
        reinterpret_cast<DWORD_PTR>(this),
        CALLBACK_FUNCTION);

    if (mm != MMSYSERR_NOERROR) {
        hWaveOut_ = nullptr;
        return false;
    }

    for (int i = 0; i < BUFFER_COUNT; ++i) {
        buffers_[i].assign(bufferBytes_, 0);
        std::memset(&headers_[i], 0, sizeof(WAVEHDR));
        headers_[i].lpData = reinterpret_cast<LPSTR>(buffers_[i].data());
        headers_[i].dwBufferLength = static_cast<DWORD>(buffers_[i].size());
        mm = waveOutPrepareHeader(hWaveOut_, &headers_[i], sizeof(WAVEHDR));
        if (mm != MMSYSERR_NOERROR) {
            return false;
        }
    }

    opened_.store(true, std::memory_order_release);

    // ポンプスレッド起動
    stop_.store(false, std::memory_order_release);
    needPump_.store(true, std::memory_order_release);
    if (!pumpThread_.joinable()) {
        pumpThread_ = std::thread(&WinMMWavePlayer::PumpThreadProc, this);
    }
    cv_.notify_one();

    return true;
}

void WinMMWavePlayer::PumpThreadProc() {
    for (;;) {
        std::vector<WAVEHDR*> toSubmit;
        toSubmit.clear();

        {
            std::unique_lock<std::mutex> lk(mtx_);
            cv_.wait(lk, [&] { return stop_.load(std::memory_order_acquire) || needPump_.load(std::memory_order_acquire); });
            if (stop_.load(std::memory_order_acquire)) break;

            // 使われていないヘッダへできる限り詰める
            for (int i = 0; i < BUFFER_COUNT; ++i) {
                auto& hdr = headers_[i];
                if (hdr.dwFlags & WHDR_INQUEUE) continue; // 再生中

                // blockAlign 未満は保留
                if (inputBuffer_.size() < wfx_.nBlockAlign) break;

                const size_t availableBlocks = inputBuffer_.size() / wfx_.nBlockAlign;
                const size_t copyBytes = std::min<size_t>(availableBlocks * wfx_.nBlockAlign, bufferBytes_);
                if (copyBytes == 0) break;

                std::memcpy(hdr.lpData, inputBuffer_.data(), copyBytes);
                hdr.dwBufferLength = static_cast<DWORD>(copyBytes);

                // 取り出した分を削除
                inputBuffer_.erase(inputBuffer_.begin(), inputBuffer_.begin() + copyBytes);

                toSubmit.push_back(&hdr);
            }

            // 今回の要求は処理済み。残データが多ければ次回も叩かれる
            needPump_.store(false, std::memory_order_release);
        }

        // waveOutWrite はミューテックス非保持で呼ぶ（再入デッドロック回避）
        for (auto* hdr : toSubmit) {
            waveOutWrite(hWaveOut_, hdr, sizeof(WAVEHDR));
        }
    }
}

void CALLBACK WinMMWavePlayer::WaveOutProc(HWAVEOUT /*hwo*/, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR /*dwParam1*/, DWORD_PTR /*dwParam2*/) {
    if (uMsg != WOM_DONE && uMsg != WOM_OPEN) return;
    auto* self = reinterpret_cast<WinMMWavePlayer*>(dwInstance);
    if (!self) return;

    // 供給要求のみ通知（重い処理・ロックはしない）
    self->needPump_.store(true, std::memory_order_release);
    self->cv_.notify_one();
}

} // namespace Audio