#pragma once

#include "audio/AudioTypes.h"

namespace shareaudio {

class IAudioCapture {
public:
    virtual ~IAudioCapture() = default;
    virtual Result<void> initialize(const AudioFormat& format, std::string device_id) = 0;
    virtual Result<void> start(PcmCallback callback) = 0;
    virtual void stop() = 0;
    virtual void shutdown() = 0;
    [[nodiscard]] virtual std::vector<AudioDevice> devices() const = 0;
};

} // namespace shareaudio
