#include "platform/SystemVolume.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#endif

namespace shareaudio {
namespace {

#ifdef _WIN32
std::string hresult_message(const char* operation, HRESULT result)
{
    std::ostringstream message;
    message << operation << " failed with HRESULT 0x"
            << std::hex << std::uppercase << static_cast<unsigned long>(result) << '.';
    return message.str();
}
#endif

} // namespace

float gain_from_decibels(float decibels, bool muted)
{
    if (muted) {
        return 0.0f;
    }
    const float gain = std::pow(10.0f, decibels / 20.0f);
    if (!std::isfinite(gain)) {
        return decibels > 0.0f ? 1.0f : 0.0f;
    }
    return std::clamp(gain, 0.0f, 1.0f);
}

bool system_volume_supported()
{
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

struct SystemVolumeReader::Impl {
#ifdef _WIN32
    bool com_owned {};
    bool com_available {};
    IMMDeviceEnumerator* enumerator {};
    IMMDevice* device {};
    IAudioEndpointVolume* endpoint_volume {};
    std::wstring endpoint_id;

    void release_endpoint()
    {
        if (endpoint_volume != nullptr) {
            endpoint_volume->Release();
            endpoint_volume = nullptr;
        }
        if (device != nullptr) {
            device->Release();
            device = nullptr;
        }
        endpoint_id.clear();
    }

    void release_all()
    {
        release_endpoint();
        if (enumerator != nullptr) {
            enumerator->Release();
            enumerator = nullptr;
        }
        if (com_owned) {
            CoUninitialize();
        }
        com_owned = false;
        com_available = false;
    }
#endif
};

SystemVolumeReader::SystemVolumeReader()
    : impl_(std::make_unique<Impl>())
{
}

SystemVolumeReader::~SystemVolumeReader()
{
#ifdef _WIN32
    impl_->release_all();
#endif
}

Result<void> SystemVolumeReader::bind(const std::wstring& endpoint_id)
{
#ifdef _WIN32
    if (endpoint_id.empty()) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, "The active WASAPI output endpoint id is empty."));
    }
    if (impl_->endpoint_volume != nullptr && impl_->endpoint_id == endpoint_id) {
        return Result<void>::success();
    }

    if (!impl_->com_available) {
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(initialized)) {
            impl_->com_owned = true;
            impl_->com_available = true;
        } else if (initialized == RPC_E_CHANGED_MODE) {
            impl_->com_available = true;
        } else {
            return Result<void>::failure(make_error(ErrorCode::AudioError, hresult_message("CoInitializeEx", initialized)));
        }
    }

    if (impl_->enumerator == nullptr) {
        const HRESULT created = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void**>(&impl_->enumerator));
        if (FAILED(created)) {
            return Result<void>::failure(make_error(ErrorCode::AudioError, hresult_message("CoCreateInstance(MMDeviceEnumerator)", created)));
        }
    }

    impl_->release_endpoint();
    HRESULT result = impl_->enumerator->GetDevice(endpoint_id.c_str(), &impl_->device);
    if (FAILED(result)) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, hresult_message("IMMDeviceEnumerator::GetDevice", result)));
    }

    result = impl_->device->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&impl_->endpoint_volume));
    if (FAILED(result)) {
        impl_->release_endpoint();
        return Result<void>::failure(make_error(ErrorCode::AudioError, hresult_message("IMMDevice::Activate(IAudioEndpointVolume)", result)));
    }

    impl_->endpoint_id = endpoint_id;
    return Result<void>::success();
#else
    (void)endpoint_id;
    return Result<void>::failure(make_error(ErrorCode::NotSupported, "System volume tracking is supported on Windows only."));
#endif
}

Result<float> SystemVolumeReader::read_gain(bool ignore_mute) const
{
#ifdef _WIN32
    if (impl_->endpoint_volume == nullptr) {
        return Result<float>::failure(make_error(ErrorCode::InvalidState, "System volume reader is not bound to an output endpoint."));
    }

    BOOL muted = FALSE;
    if (!ignore_mute) {
        const HRESULT mute_result = impl_->endpoint_volume->GetMute(&muted);
        if (FAILED(mute_result)) {
            return Result<float>::failure(make_error(ErrorCode::AudioError, hresult_message("IAudioEndpointVolume::GetMute", mute_result)));
        }
    }

    float decibels = 0.0f;
    const HRESULT result = impl_->endpoint_volume->GetMasterVolumeLevel(&decibels);
    if (FAILED(result)) {
        return Result<float>::failure(make_error(ErrorCode::AudioError, hresult_message("IAudioEndpointVolume::GetMasterVolumeLevel", result)));
    }

    return Result<float>::success(gain_from_decibels(decibels, muted != FALSE));
#else
    (void)ignore_mute;
    return Result<float>::failure(make_error(ErrorCode::NotSupported, "System volume tracking is supported on Windows only."));
#endif
}

Result<bool> SystemVolumeReader::read_muted() const
{
#ifdef _WIN32
    if (impl_->endpoint_volume == nullptr) {
        return Result<bool>::failure(make_error(ErrorCode::InvalidState, "System volume reader is not bound to an output endpoint."));
    }
    BOOL muted = FALSE;
    const HRESULT result = impl_->endpoint_volume->GetMute(&muted);
    if (FAILED(result)) {
        return Result<bool>::failure(make_error(ErrorCode::AudioError, hresult_message("IAudioEndpointVolume::GetMute", result)));
    }
    return Result<bool>::success(muted != FALSE);
#else
    return Result<bool>::failure(make_error(ErrorCode::NotSupported, "Local audio mute is supported on Windows only."));
#endif
}

Result<void> SystemVolumeReader::set_muted(bool muted) const
{
#ifdef _WIN32
    if (impl_->endpoint_volume == nullptr) {
        return Result<void>::failure(make_error(ErrorCode::InvalidState, "System volume reader is not bound to an output endpoint."));
    }
    const HRESULT result = impl_->endpoint_volume->SetMute(muted ? TRUE : FALSE, nullptr);
    if (FAILED(result)) {
        return Result<void>::failure(make_error(ErrorCode::AudioError, hresult_message("IAudioEndpointVolume::SetMute", result)));
    }
    return Result<void>::success();
#else
    (void)muted;
    return Result<void>::failure(make_error(ErrorCode::NotSupported, "Local audio mute is supported on Windows only."));
#endif
}

void SystemVolumeReader::reset()
{
#ifdef _WIN32
    impl_->release_endpoint();
#endif
}

} // namespace shareaudio
