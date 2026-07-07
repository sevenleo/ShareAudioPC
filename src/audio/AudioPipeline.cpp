#include "audio/AudioPipeline.h"

namespace shareaudio {

PcmTransmitterPipeline::PcmTransmitterPipeline(AudioMode mode, std::size_t max_queued_packets)
    : chunker_(mode)
    , max_queued_packets_(max_queued_packets)
{
}

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
        packets_.push(chunker_.pop_packet());
        ++stats_.packets_produced;
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

PcmReceiverPipeline::PcmReceiverPipeline(IAudioPlayback& playback, std::size_t jitter_capacity_bytes)
    : playback_(playback)
    , jitter_(jitter_capacity_bytes)
{
}

Result<void> PcmReceiverPipeline::start()
{
    return playback_.start();
}

Result<void> PcmReceiverPipeline::receive_pcm(std::span<const std::uint8_t> bytes)
{
    std::scoped_lock lock(mutex_);
    jitter_.push(bytes);
    bytes_received_ += bytes.size();
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
