#pragma once

#include "audio/IAudioCapture.h"
#include "audio/IAudioPlayback.h"

#if SHAREAUDIO_ENABLE_MINIAUDIO
#include <miniaudio.h>
#endif

#include <atomic>
#include <deque>
#include <mutex>

namespace shareaudio {

#if SHAREAUDIO_ENABLE_MINIAUDIO

class MiniaudioCapture final : public IAudioCapture {
public:
    MiniaudioCapture();
    ~MiniaudioCapture() override;

    Result<void> initialize(const AudioFormat& format, std::string device_id) override;
    Result<void> start(PcmCallback callback) override;
    void stop() override;
    void shutdown() override;
    [[nodiscard]] std::vector<AudioDevice> devices() const override;

private:
    static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
    static void notification_callback(const ma_device_notification* pNotification);
    void handle_data(const void* input, ma_uint32 frame_count);
    Result<void> ensure_context();
    Result<ma_device_id> find_device_id(const std::string& id) const;

    mutable std::mutex mutex_;
    ma_context context_ {};
    ma_device device_ {};
    bool context_initialized_ {};
    bool device_initialized_ {};
    AudioFormat format_;
    std::string device_id_;
    PcmCallback callback_;
};

class MiniaudioPlayback final : public IAudioPlayback {
public:
    MiniaudioPlayback();
    ~MiniaudioPlayback() override;

    Result<void> initialize(const AudioFormat& format, std::string device_id) override;
    Result<void> start() override;
    Result<void> submit(std::span<const std::uint8_t> pcm) override;
    void stop() override;
    void shutdown() override;
    [[nodiscard]] std::vector<AudioDevice> devices() const override;
    [[nodiscard]] AudioStats stats() const override;

private:
    static void data_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count);
    static void notification_callback(const ma_device_notification* pNotification);
    void fill_output(void* output, ma_uint32 frame_count);
    Result<void> ensure_context();
    Result<ma_device_id> find_device_id(const std::string& id) const;

    mutable std::mutex mutex_;
    ma_context context_ {};
    ma_device device_ {};
    bool context_initialized_ {};
    bool device_initialized_ {};
    AudioFormat format_;
    std::string device_id_;
    std::deque<std::uint8_t> buffer_;
    std::size_t max_buffer_bytes_ {};
    AudioStats stats_;
};

#endif

} // namespace shareaudio
