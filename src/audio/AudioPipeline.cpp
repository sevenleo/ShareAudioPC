#include "audio/AudioPipeline.h"
#include "protocol/Protocol.h"

namespace shareaudio {

PcmTransmitterPipeline::PcmTransmitterPipeline(AudioMode mode, std::size_t max_queued_packets)
    : mode_(mode)
    , chunker_(mode == AudioMode::Quality ? AudioMode::Quality : mode) // quality mode uses 3840 bytes chunker internally
    , max_queued_packets_(max_queued_packets)
{
    if (mode_ == AudioMode::Quality) {
        AudioFormat format;
        format.sample_rate = Defaults::sample_rate;
        format.channels = Defaults::channel_count;
        (void)encoder_.initialize(format, Defaults::opus_bitrate_bps);
    }
}

// Adjust chunker construction: if Quality mode is selected, configure chunker size to 3840 bytes
// which is exactly 20ms of stereo 16-bit PCM at 48kHz (960 * 2 * 2 = 3840 bytes).

void PcmTransmitterPipeline::on_captured_pcm(std::span<const std::uint8_t> bytes)
{
    std::scoped_lock lock(mutex_);
    stats_.bytes_captured += bytes.size();
    chunker_.push(bytes);

    while (chunker_.has_packet()) {
        if (packets_.size() >= max_queued_packets_) {
            packets_.pop();
            ++stats_.dropped_packets;
        }

        auto pcm_frame = chunker_.pop_packet();
        if (mode_ == AudioMode::Quality) {
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
}

PcmReceiverPipeline::PcmReceiverPipeline(IAudioPlayback& playback, std::size_t jitter_capacity_bytes, AudioMode mode)
    : playback_(playback)
    , jitter_(jitter_capacity_bytes)
    , mode_(mode)
{
    if (mode_ == AudioMode::Quality) {
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
    if (mode_ == AudioMode::Quality) {
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
