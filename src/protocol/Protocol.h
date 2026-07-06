#pragma once

#include "app/Config.h"
#include "app/Result.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace shareaudio {

class ProtocolWriter {
public:
    static Result<void> validate_pcm_packet(AudioMode mode, std::span<const std::uint8_t> packet);
    static Result<std::vector<std::uint8_t>> make_pcm_packet(AudioMode mode, std::span<const std::uint8_t> bytes);
    static Result<std::vector<std::uint8_t>> make_opus_packet(std::span<const std::uint8_t> frame);
    static std::array<std::uint8_t, 2> encode_opus_length(std::uint16_t length);
};

class ProtocolReader {
public:
    static Result<std::uint16_t> decode_opus_length(std::span<const std::uint8_t> header);
    static Result<void> validate_opus_frame_length(std::uint16_t length);
};

} // namespace shareaudio
