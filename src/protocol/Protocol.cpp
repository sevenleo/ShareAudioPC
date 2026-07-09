#include "protocol/Protocol.h"

#include <algorithm>

namespace shareaudio {
namespace {

constexpr std::array<std::uint8_t, 4> stream_magic { 'S', 'A', 'L', '1' };
constexpr std::uint8_t stream_version = 1;

std::uint8_t mode_to_wire(AudioMode mode)
{
    switch (mode) {
    case AudioMode::Balanced:
        return 1;
    case AudioMode::Fast:
        return 2;
    case AudioMode::Efficient:
        return 3;
    }
    return 1;
}

Result<AudioMode> mode_from_wire(std::uint8_t value)
{
    switch (value) {
    case 1:
        return Result<AudioMode>::success(AudioMode::Balanced);
    case 2:
        return Result<AudioMode>::success(AudioMode::Fast);
    case 3:
        return Result<AudioMode>::success(AudioMode::Efficient);
    default:
        return Result<AudioMode>::failure(make_error(ErrorCode::ProtocolError, "Unknown stream mode in session header."));
    }
}

void write_u32_be(std::array<std::uint8_t, ProtocolWriter::stream_header_size>& bytes, std::size_t offset, std::uint32_t value)
{
    bytes[offset] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
    bytes[offset + 1] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[offset + 2] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[offset + 3] = static_cast<std::uint8_t>(value & 0xFFU);
}

std::uint32_t read_u32_be(std::span<const std::uint8_t> bytes, std::size_t offset)
{
    return (static_cast<std::uint32_t>(bytes[offset]) << 24U)
        | (static_cast<std::uint32_t>(bytes[offset + 1]) << 16U)
        | (static_cast<std::uint32_t>(bytes[offset + 2]) << 8U)
        | static_cast<std::uint32_t>(bytes[offset + 3]);
}

Result<void> validate_stream_header(const StreamHeader& header)
{
    if (header.codec != StreamCodec::PcmS16Le && header.codec != StreamCodec::Opus) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Unknown stream codec in session header."));
    }
    if (header.channels != Defaults::channel_count) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Unsupported channel count in session header."));
    }
    if (header.bytes_per_sample != Defaults::bytes_per_sample) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Unsupported sample size in session header."));
    }
    if (header.sample_rate != Defaults::sample_rate) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Unsupported sample rate in session header."));
    }
    if (header.mode == AudioMode::Efficient) {
        if (header.codec != StreamCodec::Opus) {
            return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Efficient mode must use Opus codec."));
        }
        if (header.packet_size == 0 || header.packet_size > Defaults::max_opus_frame_bytes) {
            return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Invalid Opus packet size in session header."));
        }
        return Result<void>::success();
    }

    if (header.codec != StreamCodec::PcmS16Le) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Raw PCM modes must use pcm_s16le codec."));
    }
    if (header.packet_size != packet_size_for_mode(header.mode)) {
        return Result<void>::failure(make_error(ErrorCode::ProtocolError, "Packet size does not match stream mode."));
    }
    return Result<void>::success();
}

} // namespace

Result<std::array<std::uint8_t, ProtocolWriter::stream_header_size>> ProtocolWriter::make_stream_header(const StreamHeader& header)
{
    auto validation = validate_stream_header(header);
    if (!validation.ok()) {
        return Result<std::array<std::uint8_t, stream_header_size>>::failure(validation.error());
    }

    std::array<std::uint8_t, stream_header_size> bytes {};
    std::copy(stream_magic.begin(), stream_magic.end(), bytes.begin());
    bytes[4] = stream_version;
    bytes[5] = mode_to_wire(header.mode);
    bytes[6] = static_cast<std::uint8_t>(header.codec);
    bytes[7] = header.channels;
    bytes[8] = header.bytes_per_sample;
    bytes[9] = static_cast<std::uint8_t>((header.packet_size >> 8U) & 0xFFU);
    bytes[10] = static_cast<std::uint8_t>(header.packet_size & 0xFFU);
    bytes[11] = 0;
    write_u32_be(bytes, 12, header.sample_rate);
    return Result<std::array<std::uint8_t, stream_header_size>>::success(bytes);
}

Result<std::array<std::uint8_t, ProtocolWriter::stream_header_size>> ProtocolWriter::make_stream_header(AudioMode mode)
{
    StreamHeader header;
    header.mode = mode;
    header.codec = mode == AudioMode::Efficient ? StreamCodec::Opus : StreamCodec::PcmS16Le;
    header.packet_size = mode == AudioMode::Efficient ? Defaults::max_opus_frame_bytes : static_cast<std::uint32_t>(packet_size_for_mode(mode));
    return make_stream_header(header);
}

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

Result<StreamHeader> ProtocolReader::parse_stream_header(std::span<const std::uint8_t> bytes)
{
    if (bytes.size() != ProtocolWriter::stream_header_size) {
        return Result<StreamHeader>::failure(make_error(ErrorCode::ProtocolError, "Stream session header has an invalid size."));
    }
    if (!std::equal(stream_magic.begin(), stream_magic.end(), bytes.begin())) {
        return Result<StreamHeader>::failure(make_error(ErrorCode::ProtocolError, "Invalid stream session header magic."));
    }
    if (bytes[4] != stream_version) {
        return Result<StreamHeader>::failure(make_error(ErrorCode::ProtocolError, "Unsupported stream session header version."));
    }

    auto mode = mode_from_wire(bytes[5]);
    if (!mode.ok()) {
        return Result<StreamHeader>::failure(mode.error());
    }

    StreamHeader header;
    header.mode = mode.value();
    header.codec = static_cast<StreamCodec>(bytes[6]);
    header.channels = bytes[7];
    header.bytes_per_sample = bytes[8];
    header.packet_size = (static_cast<std::uint32_t>(bytes[9]) << 8U) | static_cast<std::uint32_t>(bytes[10]);
    header.sample_rate = read_u32_be(bytes, 12);

    auto validation = validate_stream_header(header);
    if (!validation.ok()) {
        return Result<StreamHeader>::failure(validation.error());
    }
    return Result<StreamHeader>::success(header);
}

} // namespace shareaudio
