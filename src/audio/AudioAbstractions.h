#pragma once

#include "audio/IAudioCapture.h"
#include "audio/IAudioPlayback.h"

#include <atomic>
#include <mutex>
#include <thread>

namespace shareaudio {

class GeneratedToneCapture final : public IAudioCapture {
public:
    Result<void> initialize(const AudioFormat& format, std::string device_id) override;
    Result<void> start(PcmCallback callback) override;
    void stop() override;
    void shutdown() override;
    [[nodiscard]] std::vector<AudioDevice> devices() const override;

private:
    AudioFormat format_;
    std::string device_id_;
    std::atomic_bool running_ { false };
    std::thread worker_;
};

class NullAudioPlayback final : public IAudioPlayback {
public:
    Result<void> initialize(const AudioFormat& format, std::string device_id) override;
    Result<void> start() override;
    Result<void> submit(std::span<const std::uint8_t> pcm) override;
    void stop() override;
    void shutdown() override;
    [[nodiscard]] std::vector<AudioDevice> devices() const override;
    [[nodiscard]] AudioStats stats() const override;

private:
    mutable std::mutex mutex_;
    AudioFormat format_;
    std::string device_id_;
    bool started_ {};
    AudioStats stats_;
};

} // namespace shareaudio
