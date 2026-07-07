#pragma once

#include "app/Config.h"
#include "app/Result.h"

#include <cstdint>
#include <span>
#include <vector>

namespace shareaudio {

class OpusEncoder {
public:
    OpusEncoder() = default;
    ~OpusEncoder();
    Result<void> initialize(const AudioFormat& format, int bitrate_bps);
    Result<std::vector<std::uint8_t>> encode(std::span<const std::uint8_t> pcm);

private:
    AudioFormat format_;
    int bitrate_bps_ { Defaults::opus_bitrate_bps };
    bool initialized_ {};
    void* state_ { nullptr };
};

class OpusDecoder {
public:
    OpusDecoder() = default;
    ~OpusDecoder();
    Result<void> initialize(const AudioFormat& format);
    Result<std::vector<std::uint8_t>> decode(std::span<const std::uint8_t> frame);

private:
    AudioFormat format_;
    bool initialized_ {};
    void* state_ { nullptr };
};

} // namespace shareaudio
