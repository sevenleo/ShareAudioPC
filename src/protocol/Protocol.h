#pragma once

#include "app/Config.h"
#include "app/Result.h"

#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace shareaudio {

enum class StreamCodec : std::uint8_t {
    PcmS16Le = 1,
    Opus = 2
};

struct StreamHeader {
    AudioMode mode { AudioMode::Balanced };
    StreamCodec codec { StreamCodec::PcmS16Le };
    std::uint8_t channels { Defaults::channel_count };
    std::uint8_t bytes_per_sample { Defaults::bytes_per_sample };
    std::uint32_t sample_rate { Defaults::sample_rate };
    std::uint32_t packet_size { Defaults::balanced_packet_bytes };
};

class ProtocolWriter {
public:
    static constexpr std::size_t stream_header_size = 16;

    static Result<std::array<std::uint8_t, stream_header_size>> make_stream_header(const StreamHeader& header);
    static Result<std::array<std::uint8_t, stream_header_size>> make_stream_header(AudioMode mode);
    static Result<void> validate_pcm_packet(AudioMode mode, std::span<const std::uint8_t> packet);
    static Result<std::vector<std::uint8_t>> make_pcm_packet(AudioMode mode, std::span<const std::uint8_t> bytes);
    static Result<std::vector<std::uint8_t>> make_opus_packet(std::span<const std::uint8_t> frame);
    static std::array<std::uint8_t, 2> encode_opus_length(std::uint16_t length);
};

class ProtocolReader {
public:
    static Result<StreamHeader> parse_stream_header(std::span<const std::uint8_t> bytes);
    static Result<std::uint16_t> decode_opus_length(std::span<const std::uint8_t> header);
    static Result<void> validate_opus_frame_length(std::uint16_t length);
};

} // namespace shareaudio
