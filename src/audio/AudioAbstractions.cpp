#include "audio/AudioAbstractions.h"

#include "app/Logger.h"

#include <chrono>
#include <cmath>

namespace shareaudio {

Result<void> GeneratedToneCapture::initialize(const AudioFormat& format, std::string device_id)
{
    format_ = format;
    device_id_ = std::move(device_id);
    if (format_.sample_rate <= 0 || format_.channels <= 0) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, "Invalid generated capture audio format."));
    }
    return Result<void>::success();
}

Result<void> GeneratedToneCapture::start(PcmCallback callback)
{
    if (running_) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Generated capture is already running."));
    }
    running_ = true;
    worker_ = std::thread([this, callback = std::move(callback)] {
        Logger::info("Generated tone capture started.");
        constexpr double frequency = 440.0;
        constexpr double pi = 3.14159265358979323846;
        std::uint64_t frame_index = 0;
        const int frames_per_chunk = static_cast<int>(Defaults::balanced_packet_bytes / format_.bytes_per_frame());

        while (running_) {
            PcmBytes chunk;
            chunk.reserve(static_cast<std::size_t>(frames_per_chunk * format_.bytes_per_frame()));
            for (int frame = 0; frame < frames_per_chunk; ++frame) {
                const double t = static_cast<double>(frame_index++) / static_cast<double>(format_.sample_rate);
                const auto sample = static_cast<std::int16_t>(std::sin(2.0 * pi * frequency * t) * 8000.0);
                for (int channel = 0; channel < format_.channels; ++channel) {
                    chunk.push_back(static_cast<std::uint8_t>(sample & 0xFF));
                    chunk.push_back(static_cast<std::uint8_t>((sample >> 8) & 0xFF));
                }
            }
            callback(chunk);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Logger::info("Generated tone capture stopped.");
    });
    return Result<void>::success();
}

void GeneratedToneCapture::stop()
{
    running_ = false;
    if (worker_.joinable()) {
        worker_.join();
    }
}

void GeneratedToneCapture::shutdown()
{
    stop();
}

std::vector<AudioDevice> GeneratedToneCapture::devices() const
{
    return { AudioDevice { "generated-tone", "Generated 440 Hz Test Tone", true } };
}

Result<void> NullAudioPlayback::initialize(const AudioFormat& format, std::string device_id)
{
    std::scoped_lock lock(mutex_);
    format_ = format;
    device_id_ = std::move(device_id);
    return Result<void>::success();
}

Result<void> NullAudioPlayback::start()
{
    std::scoped_lock lock(mutex_);
    started_ = true;
    return Result<void>::success();
}

Result<void> NullAudioPlayback::submit(std::span<const std::uint8_t> pcm)
{
    std::scoped_lock lock(mutex_);
    if (!started_) {
        ++stats_.underruns;
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "Playback has not been started."));
    }
    stats_.bytes_processed += pcm.size();
    return Result<void>::success();
}

void NullAudioPlayback::stop()
{
    std::scoped_lock lock(mutex_);
    started_ = false;
}

void NullAudioPlayback::shutdown()
{
    stop();
}

std::vector<AudioDevice> NullAudioPlayback::devices() const
{
    return { AudioDevice { "null", "Null Audio Sink", true } };
}

AudioStats NullAudioPlayback::stats() const
{
    std::scoped_lock lock(mutex_);
    return stats_;
}

} // namespace shareaudio
