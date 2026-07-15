#pragma once

#include "audio/IAudioCapture.h"
#include "audio/IAudioPlayback.h"
#include "codec/OpusCodec.h"
#include "protocol/JitterBuffer.h"
#include "protocol/PcmChunker.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <mutex>
#include <queue>
#include <span>
#include <vector>

namespace shareaudio {

struct TransmitterStats {
    std::size_t queued_packets {};
    std::size_t bytes_captured {};
    std::size_t packets_produced {};
    std::size_t dropped_packets {};
};

class PcmTransmitterPipeline {
public:
    explicit PcmTransmitterPipeline(
        AudioMode mode,
        std::size_t max_queued_packets = 256,
        VolumeMode volume_mode = VolumeMode::Full);

    void on_captured_pcm(std::span<const std::uint8_t> bytes);
    void set_volume_gain(float gain);
    void set_volume_mode(VolumeMode volume_mode);
    [[nodiscard]] float volume_gain() const;
    bool try_pop_packet(std::vector<std::uint8_t>& packet);
    [[nodiscard]] TransmitterStats stats() const;
    void reset();

private:
    std::vector<std::uint8_t> apply_volume_gain(std::span<const std::uint8_t> bytes);

    mutable std::mutex mutex_;
    AudioMode mode_;
    VolumeMode volume_mode_;
    PcmChunker chunker_;
    OpusEncoder encoder_;
    std::queue<std::vector<std::uint8_t>> packets_;
    std::size_t max_queued_packets_ {};
    TransmitterStats stats_;
    std::atomic<float> target_volume_gain_ { 1.0f };
    float applied_volume_gain_ { 1.0f };
    float ramp_target_gain_ { 1.0f };
    float ramp_step_ {};
    std::size_t ramp_frames_remaining_ {};
};

struct ReceiverStats {
    std::size_t bytes_received {};
    std::size_t bytes_played {};
    std::size_t playback_errors {};
    JitterBufferMetrics buffer;
    AudioStats playback;
};

class PcmReceiverPipeline {
public:
    PcmReceiverPipeline(IAudioPlayback& playback, std::size_t jitter_capacity_bytes, AudioMode mode = AudioMode::Balanced);

    Result<void> start();
    Result<void> receive_pcm(std::span<const std::uint8_t> bytes);
    Result<void> pump_playback(std::size_t byte_count);
    void reset();
    [[nodiscard]] ReceiverStats stats() const;

private:
    IAudioPlayback& playback_;
    JitterBuffer jitter_;
    mutable std::mutex mutex_;
    std::size_t bytes_received_ {};
    std::size_t bytes_played_ {};
    std::size_t playback_errors_ {};
    AudioMode mode_;
    OpusDecoder decoder_;
};

} // namespace shareaudio
