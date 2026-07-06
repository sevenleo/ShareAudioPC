#include "app/AppController.h"
#include "app/Config.h"
#include "audio/AudioAbstractions.h"
#include "codec/OpusCodec.h"
#include "platform/LocalIp.h"
#include "protocol/JitterBuffer.h"
#include "protocol/Protocol.h"
#include "storage/RecentDevices.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void test_config()
{
    shareaudio::AppConfig config;
    expect(shareaudio::validate(config).ok(), "default config is valid");
    expect(config.audio.bytes_per_frame() == 4, "stereo s16 frame is 4 bytes");
    expect(shareaudio::packet_size_for_mode(shareaudio::AudioMode::Balanced) == 2048, "balanced packet size");
    expect(shareaudio::packet_size_for_mode(shareaudio::AudioMode::Ultrafast) == 1024, "ultrafast packet size");
    expect(shareaudio::parse_audio_mode("quality") == shareaudio::AudioMode::Quality, "parse quality mode");

    config.audio.channels = 1;
    expect(!shareaudio::validate(config).ok(), "mono config is rejected");
}

void test_protocol()
{
    std::vector<std::uint8_t> balanced(2048, 1);
    std::vector<std::uint8_t> ultrafast(1024, 2);
    std::vector<std::uint8_t> wrong(7, 3);

    expect(shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Balanced, balanced).ok(), "balanced PCM packet accepted");
    expect(shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Ultrafast, ultrafast).ok(), "ultrafast PCM packet accepted");
    expect(!shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Balanced, wrong).ok(), "wrong PCM packet rejected");

    auto header = shareaudio::ProtocolWriter::encode_opus_length(513);
    expect(header[0] == 0x02 && header[1] == 0x01, "opus length is big endian");
    expect(shareaudio::ProtocolReader::decode_opus_length(header).value() == 513, "opus length decodes");
    expect(!shareaudio::ProtocolReader::decode_opus_length(std::vector<std::uint8_t> { 1 }).ok(), "short opus header rejected");
    expect(!shareaudio::ProtocolReader::validate_opus_frame_length(0).ok(), "zero opus frame length rejected");

    std::vector<std::uint8_t> opus_frame(20, 9);
    auto packet = shareaudio::ProtocolWriter::make_opus_packet(opus_frame);
    expect(packet.ok(), "opus packet is created");
    expect(packet.value().size() == 22, "opus packet includes 2 byte header");
}

void test_jitter_buffer()
{
    shareaudio::JitterBuffer buffer(8);
    buffer.push(std::vector<std::uint8_t> { 1, 2, 3, 4 });
    expect(buffer.depth_bytes() == 4, "jitter depth after push");
    auto popped = buffer.pop(2);
    expect(popped.size() == 2 && popped[0] == 1 && popped[1] == 2, "jitter pop returns bytes");
    auto underrun = buffer.pop(8);
    expect(underrun.size() == 8, "jitter underrun pads output");
    expect(buffer.metrics().underruns == 1, "jitter underrun is counted");

    buffer.push(std::vector<std::uint8_t> { 1, 2, 3, 4, 5, 6, 7, 8, 9 });
    expect(buffer.depth_bytes() == 8, "jitter keeps capacity after oversized push");
    expect(buffer.metrics().overruns >= 1, "jitter overrun is counted");
    buffer.reset();
    expect(buffer.depth_bytes() == 0 && buffer.metrics().underruns == 0, "jitter reset clears metrics");
}

void test_local_ip()
{
    expect(shareaudio::is_loopback_address("localhost"), "localhost is loopback");
    expect(shareaudio::is_loopback_address("127.0.0.1"), "127.0.0.1 is loopback");
    expect(shareaudio::is_loopback_address("::1"), "::1 is loopback");
    auto local = shareaudio::is_local_address("127.0.0.1");
    expect(local.ok() && local.value(), "127.0.0.1 is local");
}

void test_recent_devices()
{
    const auto path = std::filesystem::temp_directory_path() / "shareaudio-recent-devices-test.json";
    shareaudio::RecentDevices recent(path, 2);
    expect(recent.load().ok(), "missing recent devices file loads");
    expect(recent.add("192.168.1.10").ok(), "add first recent device");
    expect(recent.add("192.168.1.11").ok(), "add second recent device");
    expect(recent.add("192.168.1.10").ok(), "dedupe recent device");
    expect(recent.entries().size() == 2, "recent devices limit maintained");

    shareaudio::RecentDevices reloaded(path, 2);
    expect(reloaded.load().ok(), "recent devices reload");
    expect(!reloaded.entries().empty() && reloaded.entries().front() == "192.168.1.10", "recent devices order persisted");
    expect(reloaded.clear().ok(), "recent devices clear");
    std::filesystem::remove(path);
}

void test_app_controller()
{
    shareaudio::AppController controller;
    expect(controller.set_audio_mode(shareaudio::AudioMode::Ultrafast).ok(), "mode can be set while idle");
    expect(controller.start_transmitter().ok(), "transmitter can start");
    expect(!controller.connect_receiver("192.168.1.20").ok(), "receiver cannot start while transmitter active");
    expect(controller.stop_transmitter().ok(), "transmitter can stop");
    expect(!controller.connect_receiver("127.0.0.1").ok(), "self connection is blocked");
}

void test_audio_fakes()
{
    shareaudio::NullAudioPlayback playback;
    shareaudio::AudioFormat format;
    expect(playback.initialize(format, "null").ok(), "null playback initializes");
    expect(!playback.submit(std::vector<std::uint8_t> { 1, 2 }).ok(), "null playback rejects submit before start");
    expect(playback.start().ok(), "null playback starts");
    expect(playback.submit(std::vector<std::uint8_t> { 1, 2, 3, 4 }).ok(), "null playback accepts PCM");
    expect(playback.stats().bytes_processed == 4, "null playback tracks bytes");
}

void test_opus_stub()
{
    shareaudio::OpusEncoder encoder;
    shareaudio::AudioFormat format;
    auto init = encoder.initialize(format, shareaudio::Defaults::opus_bitrate_bps);
    if (init.ok()) {
        auto encoded = encoder.encode(std::vector<std::uint8_t>(3840, 0));
        expect(!encoded.ok(), "opus encode wrapper is intentionally incomplete before libopus implementation");
    } else {
        expect(init.error().code == shareaudio::ErrorCode::NotSupported, "opus reports not supported without libopus");
    }
}

} // namespace

int main()
{
    test_config();
    test_protocol();
    test_jitter_buffer();
    test_local_ip();
    test_recent_devices();
    test_app_controller();
    test_audio_fakes();
    test_opus_stub();

    if (failures != 0) {
        std::cerr << failures << " test expectation(s) failed.\n";
        return 1;
    }
    std::cout << "All ShareAudioLite tests passed.\n";
    return 0;
}
