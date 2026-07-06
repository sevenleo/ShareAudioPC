#include "protocol/Protocol.h"

#include <algorithm>

namespace shareaudio {

Result<void> ProtocolWriter::validate_pcm_packet(AudioMode mode, std::span<const std::uint8_t> packet)
{
    const auto expected = packet_size_for_mode(mode);
    if (expected == 0) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "PCM packet validation is only valid for raw PCM modes."));
    }
    if (packet.size() != expected) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "PCM packet has an invalid size for the selected mode."));
    }
    return Result<void>::success();
}

Result<std::vector<std::uint8_t>> ProtocolWriter::make_pcm_packet(AudioMode mode, std::span<const std::uint8_t> bytes)
{
    auto validation = validate_pcm_packet(mode, bytes);
    if (!validation.ok()) {
        return Result<std::vector<std::uint8_t>>::failure(validation.error());
    }
    return Result<std::vector<std::uint8_t>>::success(std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
}

std::array<std::uint8_t, 2> ProtocolWriter::encode_opus_length(std::uint16_t length)
{
    return {
        static_cast<std::uint8_t>((length >> 8U) & 0xFFU),
        static_cast<std::uint8_t>(length & 0xFFU)
    };
}

Result<std::vector<std::uint8_t>> ProtocolWriter::make_opus_packet(std::span<const std::uint8_t> frame)
{
    if (frame.empty()) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::ProtocolError, "Opus frame cannot be empty."));
    }
    if (frame.size() > Defaults::max_opus_frame_bytes || frame.size() > 65535U) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::ProtocolError, "Opus frame exceeds the maximum allowed size."));
    }

    auto header = encode_opus_length(static_cast<std::uint16_t>(frame.size()));
    std::vector<std::uint8_t> packet;
    packet.reserve(frame.size() + header.size());
    packet.insert(packet.end(), header.begin(), header.end());
    packet.insert(packet.end(), frame.begin(), frame.end());
    return Result<std::vector<std::uint8_t>>::success(std::move(packet));
}

Result<std::uint16_t> ProtocolReader::decode_opus_length(std::span<const std::uint8_t> header)
{
    if (header.size() != 2) {
        return Result<std::uint16_t>::failure(make_error(ErrorCode::ProtocolError, "Opus length header must be exactly 2 bytes."));
    }

    const auto length = static_cast<std::uint16_t>((static_cast<std::uint16_t>(header[0]) << 8U) | header[1]);
    auto validation = validate_opus_frame_length(length);
    if (!validation.ok()) {
        return Result<std::uint16_t>::failure(validation.error());
    }
    return Result<std::uint16_t>::success(length);
}

Result<void> ProtocolReader::validate_opus_frame_length(std::uint16_t length)
{
    if (length == 0) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Opus frame length cannot be zero."));
    }
    if (length > Defaults::max_opus_frame_bytes) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Opus frame length exceeds the configured maximum."));
    }
    return Result<void>::success();
}

} // namespace shareaudio
