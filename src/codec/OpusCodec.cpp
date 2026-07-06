#include "codec/OpusCodec.h"

#include "protocol/Protocol.h"

namespace shareaudio {

Result<void> OpusEncoder::initialize(const AudioFormat& format, int bitrate_bps)
{
    if (format.sample_rate != Defaults::sample_rate || format.channels != Defaults::channel_count) {
        return Result<void>::failure(make_error(ErrorCode::CodecError, "Opus encoder requires 48 kHz stereo PCM."));
    }
    if (bitrate_bps <= 0) {
        return Result<void>::failure(make_error(ErrorCode::CodecError, "Opus bitrate must be positive."));
    }

    format_ = format;
    bitrate_bps_ = bitrate_bps;
    initialized_ = true;

#if SHAREAUDIO_HAS_LIBOPUS
    return Result<void>::success();
#else
    return Result<void>::failure(make_error(ErrorCode::NotSupported, "libopus is not linked in this build."));
#endif
}

Result<std::vector<std::uint8_t>> OpusEncoder::encode(std::span<const std::uint8_t> pcm)
{
    if (!initialized_) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::InvalidState, "Opus encoder is not initialized."));
    }
    if (pcm.empty()) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::CodecError, "Cannot encode an empty PCM frame."));
    }

#if SHAREAUDIO_HAS_LIBOPUS
    (void)pcm;
    return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NotSupported, "libopus encoding is not implemented in this wrapper yet."));
#else
    (void)pcm;
    return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NotSupported, "libopus is not linked in this build."));
#endif
}

Result<void> OpusDecoder::initialize(const AudioFormat& format)
{
    if (format.sample_rate != Defaults::sample_rate || format.channels != Defaults::channel_count) {
        return Result<void>::failure(make_error(ErrorCode::CodecError, "Opus decoder requires 48 kHz stereo PCM."));
    }

    format_ = format;
    initialized_ = true;

#if SHAREAUDIO_HAS_LIBOPUS
    return Result<void>::success();
#else
    return Result<void>::failure(make_error(ErrorCode::NotSupported, "libopus is not linked in this build."));
#endif
}

Result<std::vector<std::uint8_t>> OpusDecoder::decode(std::span<const std::uint8_t> frame)
{
    if (!initialized_) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::InvalidState, "Opus decoder is not initialized."));
    }
    auto validation = ProtocolReader::validate_opus_frame_length(static_cast<std::uint16_t>(frame.size()));
    if (!validation.ok()) {
        return Result<std::vector<std::uint8_t>>::failure(validation.error());
    }

#if SHAREAUDIO_HAS_LIBOPUS
    return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NotSupported, "libopus decoding is not implemented in this wrapper yet."));
#else
    return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NotSupported, "libopus is not linked in this build."));
#endif
}

} // namespace shareaudio
