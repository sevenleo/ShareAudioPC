#if SHAREAUDIO_ENABLE_MINIAUDIO
#define MINIAUDIO_IMPLEMENTATION
#endif

#include "audio/MiniaudioBackend.h"

#if SHAREAUDIO_ENABLE_MINIAUDIO
#include "app/Logger.h"

#include <algorithm>
#include <cstring>

namespace shareaudio {
namespace {

constexpr std::size_t playback_buffer_frames = 48000;

std::string miniaudio_error(ma_result result, const std::string& context)
{
    return context + " failed with miniaudio error code " + std::to_string(result) + ".";
}

std::vector<AudioDevice> enumerate_devices_for_type(ma_device_type type)
{
    ma_context context {};
    if (ma_context_init(nullptr, 0, nullptr, &context) != MA_SUCCESS) {
        return {};
    }

    ma_device_info* playback_infos = nullptr;
    ma_device_info* capture_infos = nullptr;
    ma_uint32 playback_count = 0;
    ma_uint32 capture_count = 0;
    std::vector<AudioDevice> devices;

    if (ma_context_get_devices(&context, &playback_infos, &playback_count, &capture_infos, &capture_count) == MA_SUCCESS) {
        if (type == ma_device_type_playback || type == ma_device_type_loopback) {
            for (ma_uint32 i = 0; i < playback_count; ++i) {
                devices.push_back(AudioDevice {
                    "playback:" + std::to_string(i),
                    playback_infos[i].name,
                    playback_infos[i].isDefault != 0
                });
            }
        } else {
            for (ma_uint32 i = 0; i < capture_count; ++i) {
                devices.push_back(AudioDevice {
                    "capture:" + std::to_string(i),
                    capture_infos[i].name,
                    capture_infos[i].isDefault != 0
                });
            }
        }
    }

    ma_context_uninit(&context);
    return devices;
}

bool parse_index_id(const std::string& id, const std::string& prefix, ma_uint32& index)
{
    if (id.rfind(prefix, 0) != 0) {
        return false;
    }

    try {
        index = static_cast<ma_uint32>(std::stoul(id.substr(prefix.size())));
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

MiniaudioCapture::MiniaudioCapture() = default;

MiniaudioCapture::~MiniaudioCapture()
{
    shutdown();
}

Result<void> MiniaudioCapture::ensure_context()
{
    if (context_initialized_) {
        return Result<void>::success();
    }

    const ma_result result = ma_context_init(nullptr, 0, nullptr, &context_);
    if (result != MA_SUCCESS) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, miniaudio_error(result, "ma_context_init")));
    }
    context_initialized_ = true;
    return Result<void>::success();
}

Result<ma_device_id> MiniaudioCapture::find_device_id(const std::string& id) const
{
    ma_device_id selected {};
    if (id.empty()) {
        return Result<ma_device_id>::success(selected);
    }

    ma_uint32 index = 0;
#ifdef _WIN32
    const std::string prefix = "playback:";
#else
    const std::string prefix = "capture:";
#endif
    if (!parse_index_id(id, prefix, index)) {
        return Result<ma_device_id>::failure(make_error(ErrorCode::AudioError, "Unsupported capture device id: " + id));
    }

    ma_device_info* playback_infos = nullptr;
    ma_device_info* capture_infos = nullptr;
    ma_uint32 playback_count = 0;
    ma_uint32 capture_count = 0;
    if (ma_context_get_devices(const_cast<ma_context*>(&context_), &playback_infos, &playback_count, &capture_infos, &capture_count) != MA_SUCCESS) {
        return Result<ma_device_id>::failure(make_error(ErrorCode::AudioError, "Unable to enumerate miniaudio devices."));
    }

#ifdef _WIN32
    if (index >= playback_count) {
        return Result<ma_device_id>::failure(make_error(ErrorCode::AudioError, "Selected loopback playback device was not found."));
    }
    selected = playback_infos[index].id;
#else
    if (index >= capture_count) {
        return Result<ma_device_id>::failure(make_error(ErrorCode::AudioError, "Selected capture device was not found."));
    }
    selected = capture_infos[index].id;
#endif
    return Result<ma_device_id>::success(selected);
}

Result<void> MiniaudioCapture::initialize(const AudioFormat& format, std::string device_id)
{
    std::scoped_lock lock(mutex_);
    shutdown();
    format_ = format;
    device_id_ = std::move(device_id);

    auto context = ensure_context();
    if (!context.ok()) {
        return context;
    }

#ifdef _WIN32
    const ma_device_type device_type = ma_device_type_loopback;
#else
    const ma_device_type device_type = ma_device_type_capture;
#endif

    ma_device_config config = ma_device_config_init(device_type);
    config.capture.format = ma_format_s16;
    config.capture.channels = static_cast<ma_uint32>(format_.channels);
    config.sampleRate = static_cast<ma_uint32>(format_.sample_rate);
    config.dataCallback = &MiniaudioCapture::data_callback;
    config.pUserData = this;

    ma_device_id selected {};
    if (!device_id_.empty()) {
        auto selected_result = find_device_id(device_id_);
        if (!selected_result.ok()) {
            return Result<void>::failure(selected_result.error());
        }
        selected = selected_result.value();
        config.capture.pDeviceID = &selected;
    }

    const ma_result result = ma_device_init(&context_, &config, &device_);
    if (result != MA_SUCCESS) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, miniaudio_error(result, "ma_device_init capture")));
    }

    device_initialized_ = true;
    Logger::info("miniaudio capture initialized.");
    return Result<void>::success();
}

Result<void> MiniaudioCapture::start(PcmCallback callback)
{
    std::scoped_lock lock(mutex_);
    if (!device_initialized_) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "miniaudio capture is not initialized."));
    }
    callback_ = std::move(callback);
    const ma_result result = ma_device_start(&device_);
    if (result != MA_SUCCESS) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, miniaudio_error(result, "ma_device_start capture")));
    }
    Logger::info("miniaudio capture started.");
    return Result<void>::success();
}

void MiniaudioCapture::stop()
{
    std::scoped_lock lock(mutex_);
    if (device_initialized_) {
        ma_device_stop(&device_);
    }
}

void MiniaudioCapture::shutdown()
{
    if (device_initialized_) {
        ma_device_uninit(&device_);
        device_initialized_ = false;
    }
    if (context_initialized_) {
        ma_context_uninit(&context_);
        context_initialized_ = false;
    }
}

std::vector<AudioDevice> MiniaudioCapture::devices() const
{
#ifdef _WIN32
    return enumerate_devices_for_type(ma_device_type_loopback);
#else
    return enumerate_devices_for_type(ma_device_type_capture);
#endif
}

void MiniaudioCapture::data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    (void)output;
    auto* self = static_cast<MiniaudioCapture*>(device->pUserData);
    if (self != nullptr) {
        self->handle_data(input, frame_count);
    }
}

void MiniaudioCapture::handle_data(const void* input, ma_uint32 frame_count)
{
    if (input == nullptr || !callback_) {
        return;
    }

    const auto byte_count = static_cast<std::size_t>(frame_count) * static_cast<std::size_t>(format_.bytes_per_frame());
    callback_(std::span<const std::uint8_t>(static_cast<const std::uint8_t*>(input), byte_count));
}

MiniaudioPlayback::MiniaudioPlayback() = default;

MiniaudioPlayback::~MiniaudioPlayback()
{
    shutdown();
}

Result<void> MiniaudioPlayback::ensure_context()
{
    if (context_initialized_) {
        return Result<void>::success();
    }

    const ma_result result = ma_context_init(nullptr, 0, nullptr, &context_);
    if (result != MA_SUCCESS) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, miniaudio_error(result, "ma_context_init")));
    }
    context_initialized_ = true;
    return Result<void>::success();
}

Result<ma_device_id> MiniaudioPlayback::find_device_id(const std::string& id) const
{
    ma_device_id selected {};
    if (id.empty()) {
        return Result<ma_device_id>::success(selected);
    }

    ma_uint32 index = 0;
    if (!parse_index_id(id, "playback:", index)) {
        return Result<ma_device_id>::failure(make_error(ErrorCode::AudioError, "Unsupported playback device id: " + id));
    }

    ma_device_info* playback_infos = nullptr;
    ma_uint32 playback_count = 0;
    if (ma_context_get_devices(const_cast<ma_context*>(&context_), &playback_infos, &playback_count, nullptr, nullptr) != MA_SUCCESS) {
        return Result<ma_device_id>::failure(make_error(ErrorCode::AudioError, "Unable to enumerate miniaudio playback devices."));
    }
    if (index >= playback_count) {
        return Result<ma_device_id>::failure(make_error(ErrorCode::AudioError, "Selected playback device was not found."));
    }

    selected = playback_infos[index].id;
    return Result<ma_device_id>::success(selected);
}

Result<void> MiniaudioPlayback::initialize(const AudioFormat& format, std::string device_id)
{
    std::scoped_lock lock(mutex_);
    shutdown();
    format_ = format;
    device_id_ = std::move(device_id);
    max_buffer_bytes_ = playback_buffer_frames * static_cast<std::size_t>(format_.bytes_per_frame());

    auto context = ensure_context();
    if (!context.ok()) {
        return context;
    }

    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format = ma_format_s16;
    config.playback.channels = static_cast<ma_uint32>(format_.channels);
    config.sampleRate = static_cast<ma_uint32>(format_.sample_rate);
    config.dataCallback = &MiniaudioPlayback::data_callback;
    config.pUserData = this;

    ma_device_id selected {};
    if (!device_id_.empty()) {
        auto selected_result = find_device_id(device_id_);
        if (!selected_result.ok()) {
            return Result<void>::failure(selected_result.error());
        }
        selected = selected_result.value();
        config.playback.pDeviceID = &selected;
    }

    const ma_result result = ma_device_init(&context_, &config, &device_);
    if (result != MA_SUCCESS) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, miniaudio_error(result, "ma_device_init playback")));
    }

    device_initialized_ = true;
    Logger::info("miniaudio playback initialized.");
    return Result<void>::success();
}

Result<void> MiniaudioPlayback::start()
{
    std::scoped_lock lock(mutex_);
    if (!device_initialized_) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "miniaudio playback is not initialized."));
    }
    const ma_result result = ma_device_start(&device_);
    if (result != MA_SUCCESS) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, miniaudio_error(result, "ma_device_start playback")));
    }
    Logger::info("miniaudio playback started.");
    return Result<void>::success();
}

Result<void> MiniaudioPlayback::submit(std::span<const std::uint8_t> pcm)
{
    std::scoped_lock lock(mutex_);
    if (!device_initialized_) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "miniaudio playback is not initialized."));
    }

    if (buffer_.size() + pcm.size() > max_buffer_bytes_) {
        const auto excess = buffer_.size() + pcm.size() - max_buffer_bytes_;
        for (std::size_t i = 0; i < excess && !buffer_.empty(); ++i) {
            buffer_.pop_front();
        }
        ++stats_.overruns;
    }

    buffer_.insert(buffer_.end(), pcm.begin(), pcm.end());
    stats_.bytes_processed += pcm.size();
    return Result<void>::success();
}

void MiniaudioPlayback::stop()
{
    std::scoped_lock lock(mutex_);
    if (device_initialized_) {
        ma_device_stop(&device_);
    }
}

void MiniaudioPlayback::shutdown()
{
    if (device_initialized_) {
        ma_device_uninit(&device_);
        device_initialized_ = false;
    }
    if (context_initialized_) {
        ma_context_uninit(&context_);
        context_initialized_ = false;
    }
    buffer_.clear();
}

std::vector<AudioDevice> MiniaudioPlayback::devices() const
{
    return enumerate_devices_for_type(ma_device_type_playback);
}

AudioStats MiniaudioPlayback::stats() const
{
    std::scoped_lock lock(mutex_);
    return stats_;
}

void MiniaudioPlayback::data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    (void)input;
    auto* self = static_cast<MiniaudioPlayback*>(device->pUserData);
    if (self != nullptr) {
        self->fill_output(output, frame_count);
    }
}

void MiniaudioPlayback::fill_output(void* output, ma_uint32 frame_count)
{
    const auto byte_count = static_cast<std::size_t>(frame_count) * static_cast<std::size_t>(format_.bytes_per_frame());
    auto* out = static_cast<std::uint8_t*>(output);

    std::scoped_lock lock(mutex_);
    std::size_t written = 0;
    while (written < byte_count && !buffer_.empty()) {
        out[written++] = buffer_.front();
        buffer_.pop_front();
    }

    if (written < byte_count) {
        std::memset(out + written, 0, byte_count - written);
        ++stats_.underruns;
    }
}

} // namespace shareaudio

#endif
