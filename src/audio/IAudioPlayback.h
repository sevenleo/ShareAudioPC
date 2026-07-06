#pragma once

#include "audio/AudioTypes.h"

namespace shareaudio {

class IAudioPlayback {
public:
    virtual ~IAudioPlayback() = default;
    virtual Result<void> initialize(const AudioFormat& format, std::string device_id) = 0;
    virtual Result<void> start() = 0;
    virtual Result<void> submit(std::span<const std::uint8_t> pcm) = 0;
    virtual void stop() = 0;
    virtual void shutdown() = 0;
    [[nodiscard]] virtual std::vector<AudioDevice> devices() const = 0;
    [[nodiscard]] virtual AudioStats stats() const = 0;
};

} // namespace shareaudio
