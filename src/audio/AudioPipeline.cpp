#include "audio/AudioPipeline.h"
#include "protocol/Protocol.h"

#include <algorithm>
#include <cmath>

namespace shareaudio {
namespace {

constexpr std::size_t stereo_frame_bytes = Defaults::channel_count * Defaults::bytes_per_sample;
constexpr std::size_t volume_ramp_frames = Defaults::sample_rate / 100;

} // namespace

PcmTransmitterPipeline::PcmTransmitterPipeline(AudioMode mode, std::size_t max_queued_packets, VolumeMode volume_mode)
    : mode_(mode)
    , volume_mode_(volume_mode)
    , chunker_(mode == AudioMode::Efficient ? Defaults::opus_pcm_frame_bytes : packet_size_for_mode(mode))
    , max_queued_packets_(max_queued_packets)
{
    if (mode_ == AudioMode::Efficient) {
        AudioFormat format;
        format.sample_rate = Defaults::sample_rate;
        format.channels = Defaults::channel_count;
        (void)encoder_.initialize(format, Defaults::opus_bitrate_bps);
    }
}

void PcmTransmitterPipeline::on_captured_pcm(std::span<const std::uint8_t> bytes)
{
    std::scoped_lock lock(mutex_);
    stats_.bytes_captured += bytes.size();
    if (volume_mode_ == VolumeMode::System) {
        const auto adjusted = apply_volume_gain(bytes);
        chunker_.push(adjusted);
    } else {
        chunker_.push(bytes);
    }

    while (chunker_.has_packet()) {
        if (packets_.size() >= max_queued_packets_) {
            packets_.pop();
            ++stats_.dropped_packets;
        }

        auto pcm_frame = chunker_.pop_packet();
        if (mode_ == AudioMode::Efficient) {
            auto encoded = encoder_.encode(pcm_frame);
            if (encoded.ok()) {
                auto wrapped = ProtocolWriter::make_opus_packet(encoded.value());
                if (wrapped.ok()) {
                    packets_.push(std::move(wrapped.value()));
                    ++stats_.packets_produced;
                } else {
                    ++stats_.dropped_packets;
                }
            } else {
                ++stats_.dropped_packets;
            }
        } else {
            packets_.push(std::move(pcm_frame));
            ++stats_.packets_produced;
        }
    }
    stats_.queued_packets = packets_.size();
}

void PcmTransmitterPipeline::set_volume_gain(float gain)
{
    if (!std::isfinite(gain)) {
        gain = 1.0f;
    }
    target_volume_gain_.store(std::clamp(gain, 0.0f, 1.0f), std::memory_order_relaxed);
}

float PcmTransmitterPipeline::volume_gain() const
{
    return target_volume_gain_.load(std::memory_order_relaxed);
}

std::vector<std::uint8_t> PcmTransmitterPipeline::apply_volume_gain(std::span<const std::uint8_t> bytes)
{
    std::vector<std::uint8_t> adjusted(bytes.begin(), bytes.end());
    const float target = target_volume_gain_.load(std::memory_order_relaxed);
    if (std::abs(target - ramp_target_gain_) > 0.000001f) {
        ramp_target_gain_ = target;
        ramp_frames_remaining_ = volume_ramp_frames;
        ramp_step_ = (ramp_target_gain_ - applied_volume_gain_) / static_cast<float>(volume_ramp_frames);
    }

    for (std::size_t frame_offset = 0; frame_offset + stereo_frame_bytes <= adjusted.size(); frame_offset += stereo_frame_bytes) {
        const float frame_gain = applied_volume_gain_;
        for (std::size_t channel = 0; channel < Defaults::channel_count; ++channel) {
            const std::size_t offset = frame_offset + channel * Defaults::bytes_per_sample;
            const std::uint16_t raw = static_cast<std::uint16_t>(adjusted[offset])
                | (static_cast<std::uint16_t>(adjusted[offset + 1]) << 8);
            const std::int32_t sample = raw <= 0x7FFFu
                ? static_cast<std::int32_t>(raw)
                : static_cast<std::int32_t>(raw) - 0x10000;
            const auto scaled = std::clamp(
                static_cast<long>(std::lround(static_cast<float>(sample) * frame_gain)),
                -32768L,
                32767L);
            const auto encoded = static_cast<std::uint16_t>(static_cast<std::int16_t>(scaled));
            adjusted[offset] = static_cast<std::uint8_t>(encoded & 0xFFu);
            adjusted[offset + 1] = static_cast<std::uint8_t>((encoded >> 8) & 0xFFu);
        }

        if (ramp_frames_remaining_ > 0) {
            applied_volume_gain_ += ramp_step_;
            --ramp_frames_remaining_;
            if (ramp_frames_remaining_ == 0) {
                applied_volume_gain_ = ramp_target_gain_;
            }
        }
    }
    return adjusted;
}

bool PcmTransmitterPipeline::try_pop_packet(std::vector<std::uint8_t>& packet)
{
    std::scoped_lock lock(mutex_);
    if (packets_.empty()) {
        stats_.queued_packets = 0;
        return false;
    }
    packet = std::move(packets_.front());
    packets_.pop();
    stats_.queued_packets = packets_.size();
    return true;
}

TransmitterStats PcmTransmitterPipeline::stats() const
{
    std::scoped_lock lock(mutex_);
    return stats_;
}

void PcmTransmitterPipeline::reset()
{
    std::scoped_lock lock(mutex_);
    chunker_.reset();
    packets_ = {};
    stats_ = {};
    target_volume_gain_.store(1.0f, std::memory_order_relaxed);
    applied_volume_gain_ = 1.0f;
    ramp_target_gain_ = 1.0f;
    ramp_step_ = 0.0f;
    ramp_frames_remaining_ = 0;
}

PcmReceiverPipeline::PcmReceiverPipeline(IAudioPlayback& playback, std::size_t jitter_capacity_bytes, AudioMode mode)
    : playback_(playback)
    , jitter_(jitter_capacity_bytes)
    , mode_(mode)
{
    if (mode_ == AudioMode::Efficient) {
        AudioFormat format;
        format.sample_rate = Defaults::sample_rate;
        format.channels = Defaults::channel_count;
        (void)decoder_.initialize(format);
    }
}

Result<void> PcmReceiverPipeline::start()
{
    return playback_.start();
}

Result<void> PcmReceiverPipeline::receive_pcm(std::span<const std::uint8_t> bytes)
{
    std::scoped_lock lock(mutex_);
    if (mode_ == AudioMode::Efficient) {
        auto decoded = decoder_.decode(bytes);
        if (!decoded.ok()) {
            return Result<void>::failure(decoded.error());
        }
        jitter_.push(decoded.value());
        bytes_received_ += bytes.size();
    } else {
        jitter_.push(bytes);
        bytes_received_ += bytes.size();
    }
    return Result<void>::success();
}

Result<void> PcmReceiverPipeline::pump_playback(std::size_t byte_count)
{
    auto chunk = jitter_.pop(byte_count);
    auto result = playback_.submit(chunk);

    std::scoped_lock lock(mutex_);
    if (!result.ok()) {
        ++playback_errors_;
        return result;
    }
    bytes_played_ += chunk.size();
    return Result<void>::success();
}

void PcmReceiverPipeline::reset()
{
    std::scoped_lock lock(mutex_);
    jitter_.reset();
    bytes_received_ = 0;
    bytes_played_ = 0;
    playback_errors_ = 0;
}

ReceiverStats PcmReceiverPipeline::stats() const
{
    std::scoped_lock lock(mutex_);
    return ReceiverStats {
        bytes_received_,
        bytes_played_,
        playback_errors_,
        jitter_.metrics(),
        playback_.stats()
    };
}

} // namespace shareaudio
