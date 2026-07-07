#include "codec/OpusCodec.h"
#include "protocol/Protocol.h"

#if SHAREAUDIO_HAS_LIBOPUS
#include <opus.h>
#include <cstring>
#endif

namespace shareaudio {

OpusEncoder::~OpusEncoder()
{
#if SHAREAUDIO_HAS_LIBOPUS
    if (state_ != nullptr) {
        opus_encoder_destroy(static_cast<::OpusEncoder*>(state_));
        state_ = nullptr;
    }
#endif
}

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
    int error = OPUS_OK;
    state_ = opus_encoder_create(
        format.sample_rate,
        format.channels,
        OPUS_APPLICATION_AUDIO,
        &error
    );
    if (error != OPUS_OK || state_ == nullptr) {
        state_ = nullptr;
        return Result<void>::failure(make_error(ErrorCode::CodecError, "Failed to create Opus encoder: error code " + std::to_string(error)));
    }

    error = opus_encoder_ctl(static_cast<::OpusEncoder*>(state_), OPUS_SET_BITRATE(bitrate_bps_));
    if (error != OPUS_OK) {
        opus_encoder_destroy(static_cast<::OpusEncoder*>(state_));
        state_ = nullptr;
        return Result<void>::failure(make_error(ErrorCode::CodecError, "Failed to set Opus bitrate: error code " + std::to_string(error)));
    }

    error = opus_encoder_ctl(static_cast<::OpusEncoder*>(state_), OPUS_SET_VBR(0)); // CBR mode as per idea.md
    if (error != OPUS_OK) {
        opus_encoder_destroy(static_cast<::OpusEncoder*>(state_));
        state_ = nullptr;
        return Result<void>::failure(make_error(ErrorCode::CodecError, "Failed to configure Opus CBR: error code " + std::to_string(error)));
    }

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
    const auto bytes_per_sample = 2;
    const auto bytes_per_frame = format_.channels * bytes_per_sample;
    if (pcm.size() % bytes_per_frame != 0) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::CodecError, "PCM input size is not aligned to frame boundary."));
    }

    const int frame_size = static_cast<int>(pcm.size() / bytes_per_frame);
    if (frame_size != 120 && frame_size != 240 && frame_size != 480 &&
        frame_size != 960 && frame_size != 1920 && frame_size != 2880) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::CodecError, "Invalid Opus frame size: " + std::to_string(frame_size)));
    }

    std::vector<std::uint8_t> output_buffer(pcm.size());
    const auto* pcm_data = reinterpret_cast<const opus_int16*>(pcm.data());

    opus_int32 bytes_encoded = opus_encode(
        static_cast<::OpusEncoder*>(state_),
        pcm_data,
        frame_size,
        output_buffer.data(),
        static_cast<opus_int32>(output_buffer.size())
    );

    if (bytes_encoded < 0) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::CodecError, "Opus encode failed with error: " + std::to_string(bytes_encoded)));
    }

    output_buffer.resize(static_cast<std::size_t>(bytes_encoded));
    return Result<std::vector<std::uint8_t>>::success(output_buffer);
#else
    (void)pcm;
    return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NotSupported, "libopus is not linked in this build."));
#endif
}

OpusDecoder::~OpusDecoder()
{
#if SHAREAUDIO_HAS_LIBOPUS
    if (state_ != nullptr) {
        opus_decoder_destroy(static_cast<::OpusDecoder*>(state_));
        state_ = nullptr;
    }
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
    int error = OPUS_OK;
    state_ = opus_decoder_create(
        format.sample_rate,
        format.channels,
        &error
    );
    if (error != OPUS_OK || state_ == nullptr) {
        state_ = nullptr;
        return Result<void>::failure(make_error(ErrorCode::CodecError, "Failed to create Opus decoder: error code " + std::to_string(error)));
    }
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
    const int max_frame_size = 5760; // 120ms max Opus frame duration at 48kHz
    std::vector<opus_int16> pcm_output(max_frame_size * format_.channels);

    int samples_decoded = opus_decode(
        static_cast<::OpusDecoder*>(state_),
        frame.data(),
        static_cast<opus_int32>(frame.size()),
        pcm_output.data(),
        max_frame_size,
        0
    );

    if (samples_decoded < 0) {
        return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::CodecError, "Opus decode failed with error: " + std::to_string(samples_decoded)));
    }

    const auto byte_count = static_cast<std::size_t>(samples_decoded) * format_.channels * sizeof(opus_int16);
    std::vector<std::uint8_t> output_bytes(byte_count);
    std::memcpy(output_bytes.data(), pcm_output.data(), byte_count);

    return Result<std::vector<std::uint8_t>>::success(output_bytes);
#else
    return Result<std::vector<std::uint8_t>>::failure(make_error(ErrorCode::NotSupported, "libopus is not linked in this build."));
#endif
}

} // namespace shareaudio
