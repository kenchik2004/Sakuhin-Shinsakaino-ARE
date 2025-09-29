#include "WinMMRecorder.h"
#include <cstring>
#include <algorithm>



namespace Audio {

static uint32_t BytesPerSample(uint16_t bitsPerSample) {
    return static_cast<uint32_t>(std::max<uint16_t>(8, bitsPerSample) / 8);
}

WinMMRecorder::WinMMRecorder(uint32_t sampleRate, uint16_t bitsPerSample, uint16_t channels, uint32_t bufferMillis)
    : bufferMillis_(bufferMillis) {
    std::memset(&wfx_, 0, sizeof(wfx_));
    wfx_.wFormatTag = WAVE_FORMAT_PCM;
    wfx_.nChannels = channels;
    wfx_.nSamplesPerSec = sampleRate;
    wfx_.wBitsPerSample = bitsPerSample;
    wfx_.nBlockAlign = static_cast<WORD>(channels * BytesPerSample(bitsPerSample));
    wfx_.nAvgBytesPerSec = wfx_.nSamplesPerSec * wfx_.nBlockAlign;

    const uint32_t bytesPerBuffer = std::max<uint32_t>(wfx_.nAvgBytesPerSec * bufferMillis_ / 1000, wfx_.nBlockAlign);
    for (int i = 0; i < 2; ++i) {
        buffers_[i].resize(bytesPerBuffer, 0);
        std::memset(&headers_[i], 0, sizeof(WAVEHDR));
        headers_[i].lpData = reinterpret_cast<LPSTR>(buffers_[i].data());
        headers_[i].dwBufferLength = static_cast<DWORD>(buffers_[i].size());
    }
}

WinMMRecorder::~WinMMRecorder() {
    Stop();
}

bool WinMMRecorder::Start() {
    if (running_.load(std::memory_order_acquire)) return true;

    MMRESULT mm = waveInOpen(
        &hWaveIn_,
        WAVE_MAPPER,
        &wfx_,
        reinterpret_cast<DWORD_PTR>(&WinMMRecorder::WaveInProc),
        reinterpret_cast<DWORD_PTR>(this),
        CALLBACK_FUNCTION);
    if (mm != MMSYSERR_NOERROR) {
        hWaveIn_ = nullptr;
        return false;
    }

    headerEmitted_.store(false, std::memory_order_release);
    stopping_.store(false, std::memory_order_release);

    for (int i = 0; i < 2; ++i) {
        mm = waveInPrepareHeader(hWaveIn_, &headers_[i], sizeof(WAVEHDR));
        if (mm != MMSYSERR_NOERROR) return false;
        if (!QueueBuffer(static_cast<size_t>(i))) return false;
    }

    mm = waveInStart(hWaveIn_);
    if (mm != MMSYSERR_NOERROR) return false;

    running_.store(true, std::memory_order_release);
    return true;
}

void WinMMRecorder::Stop() {
    if (!hWaveIn_) {
        running_.store(false, std::memory_order_release);
        return;
    }

    stopping_.store(true, std::memory_order_release);
    waveInStop(hWaveIn_);
    waveInReset(hWaveIn_);

    for (int i = 0; i < 2; ++i) {
        if (headers_[i].dwFlags & WHDR_PREPARED) {
            waveInUnprepareHeader(hWaveIn_, &headers_[i], sizeof(WAVEHDR));
        }
    }

    waveInClose(hWaveIn_);
    hWaveIn_ = nullptr;
    running_.store(false, std::memory_order_release);
}

bool WinMMRecorder::QueueBuffer(size_t index) {
    if (!hWaveIn_) return false;
    auto& hdr = headers_[index];

    // 再投入前に録音済みバイト数をクリア
    hdr.dwBytesRecorded = 0;
    hdr.dwFlags &= ~(WHDR_DONE);

    const MMRESULT mm = waveInAddBuffer(hWaveIn_, &hdr, sizeof(WAVEHDR));
    return (mm == MMSYSERR_NOERROR);
}

void CALLBACK WinMMRecorder::WaveInProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR /*dwParam2*/) {
    auto* self = reinterpret_cast<WinMMRecorder*>(dwInstance);
    if (!self) return;

    switch (uMsg) {
        case WIM_OPEN:
            self->EmitWaveHeaderIfNeeded();
            break;
        case WIM_DATA: {
            auto* pHdr = reinterpret_cast<WAVEHDR*>(dwParam1);
            if (pHdr && (pHdr->dwBytesRecorded > 0) && self->callback_) {
                // PCM チャンクを通知
                self->callback_(reinterpret_cast<const uint8_t*>(pHdr->lpData),
                                static_cast<size_t>(pHdr->dwBytesRecorded),
                                false);
            }
            // 停止中でなければ再投入
            if (!self->stopping_.load(std::memory_order_acquire)) {
                waveInAddBuffer(hwi, pHdr, sizeof(WAVEHDR));
            }
            break;
        }
        default:
            break;
    }
}

void WinMMRecorder::EmitWaveHeaderIfNeeded() {
    if (headerEmitted_.exchange(true, std::memory_order_acq_rel)) return;
    if (!callback_) return;

    // ストリーミング用途: data サイズ 0 のヘッダをまず一度だけ送る
    auto header = BuildWaveHeader(/*dataBytesPlaceholder*/ 0);
    callback_(header.data(), header.size(), true);
}

std::vector<uint8_t> WinMMRecorder::BuildWaveHeader(uint32_t dataBytesPlaceholder) const {
    // 典型的な PCM WAVE (RIFF) ヘッダを構築
    // RIFF chunk size = 36 + data size
    const uint32_t riffSize = 36u + dataBytesPlaceholder;

    struct RIFFHeader {
        char     riff[4];    // 'RIFF'
        uint32_t size;       // 36 + dataSize
        char     wave[4];    // 'WAVE'
        char     fmt_[4];    // 'fmt '
        uint32_t fmtSize;    // 16 for PCM
        uint16_t audioFormat;// 1 = PCM
        uint16_t numChannels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
        char     dataID[4];  // 'data'
        uint32_t dataSize;   // dataSize
    } header{};

    std::memcpy(header.riff,  "RIFF", 4);
    std::memcpy(header.wave,  "WAVE", 4);
    std::memcpy(header.fmt_,  "fmt ", 4);
    std::memcpy(header.dataID,"data", 4);

    header.size         = riffSize;
    header.fmtSize      = 16;
    header.audioFormat  = 1;
    header.numChannels  = wfx_.nChannels;
    header.sampleRate   = wfx_.nSamplesPerSec;
    header.byteRate     = wfx_.nAvgBytesPerSec;
    header.blockAlign   = wfx_.nBlockAlign;
    header.bitsPerSample= wfx_.wBitsPerSample;
    header.dataSize     = dataBytesPlaceholder;

    std::vector<uint8_t> out(sizeof(RIFFHeader));
    std::memcpy(out.data(), &header, sizeof(RIFFHeader));
    return out;
}

} // namespace Audio


