#pragma once

#include "app/Config.h"
#include "app/Result.h"

#include <cstdint>
#include <span>
#include <vector>

namespace shareaudio {

class OpusEncoder {
public:
    Result<void> initialize(const AudioFormat& format, int bitrate_bps);
    Result<std::vector<std::uint8_t>> encode(std::span<const std::uint8_t> pcm);

private:
    AudioFormat format_;
    int bitrate_bps_ { Defaults::opus_bitrate_bps };
    bool initialized_ {};
};

class OpusDecoder {
public:
    Result<void> initialize(const AudioFormat& format);
    Result<std::vector<std::uint8_t>> decode(std::span<const std::uint8_t> frame);

private:
    AudioFormat format_;
    bool initialized_ {};
};

} // namespace shareaudio
