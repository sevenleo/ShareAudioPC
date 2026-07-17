#include "app/AppController.h"
#include "app/Config.h"
#include "app/SessionController.h"
#include "audio/AudioAbstractions.h"
#include "audio/AudioPipeline.h"
#include "codec/OpusCodec.h"
#include "network/LoopbackTest.h"
#include "network/PcmBroadcastServer.h"
#include "platform/LocalIp.h"
#include "platform/SystemVolume.h"
#include "protocol/JitterBuffer.h"
#include "protocol/PcmChunker.h"
#include "protocol/Protocol.h"
#include "storage/RecentDevices.h"
#include "app/SingleInstance.h"
#include "app/StartupConfig.h"
#include "ui/ConsoleUi.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>
#include <cstring>

namespace {

int failures = 0;

void expect(bool condition, const std::string& message)
{
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::int16_t pcm_sample_at(const std::vector<std::uint8_t>& pcm, std::size_t byte_offset)
{
    const std::uint16_t raw = static_cast<std::uint16_t>(pcm[byte_offset])
        | (static_cast<std::uint16_t>(pcm[byte_offset + 1]) << 8);
    return static_cast<std::int16_t>(raw);
}

std::vector<std::uint8_t> constant_stereo_pcm(std::size_t frame_count, std::int16_t sample)
{
    std::vector<std::uint8_t> pcm(frame_count * 4);
    const auto encoded = static_cast<std::uint16_t>(sample);
    for (std::size_t offset = 0; offset < pcm.size(); offset += 2) {
        pcm[offset] = static_cast<std::uint8_t>(encoded & 0xFFu);
        pcm[offset + 1] = static_cast<std::uint8_t>((encoded >> 8) & 0xFFu);
    }
    return pcm;
}

void test_config()
{
    shareaudio::AppConfig config;
    expect(shareaudio::validate(config).ok(), "default config is valid");
    expect(shareaudio::Defaults::tcp_port == 33777, "default TCP port is 33777");
    expect(config.transmitter.network.port == shareaudio::Defaults::tcp_port, "default transmitter port uses default TCP port");
    expect(config.receiver.port == shareaudio::Defaults::tcp_port, "default receiver port uses default TCP port");
    expect(config.audio.bytes_per_frame() == 4, "stereo s16 frame is 4 bytes");
    expect(config.transmitter.volume_mode == shareaudio::VolumeMode::Full, "default VolumeMode is full");
    expect(shareaudio::packet_size_for_mode(shareaudio::AudioMode::Balanced) == 2048, "balanced packet size");
    expect(shareaudio::packet_size_for_mode(shareaudio::AudioMode::Fast) == 1024, "fast packet size");
    expect(shareaudio::packet_size_for_mode(shareaudio::AudioMode::Efficient) == 0, "efficient is not a raw PCM packet mode");
    expect(shareaudio::Defaults::opus_pcm_frame_bytes == 3840, "efficient opus PCM input frame is 20 ms");
    expect(shareaudio::to_string(shareaudio::AudioMode::Balanced) == "balanced", "balanced mode string");
    expect(shareaudio::to_string(shareaudio::AudioMode::Fast) == "fast", "fast mode string");
    expect(shareaudio::to_string(shareaudio::AudioMode::Efficient) == "efficient", "efficient mode string");
    expect(shareaudio::parse_audio_mode("balanced") == shareaudio::AudioMode::Balanced, "parse balanced mode");
    expect(shareaudio::parse_audio_mode("fast") == shareaudio::AudioMode::Fast, "parse fast mode");
    expect(shareaudio::parse_audio_mode("efficient") == shareaudio::AudioMode::Efficient, "parse efficient mode");
    expect(!shareaudio::parse_audio_mode("ultrafast").has_value(), "old ultrafast mode is rejected");
    expect(!shareaudio::parse_audio_mode("quality").has_value(), "old quality mode is rejected");
    expect(shareaudio::to_string(shareaudio::VolumeMode::Full) == "full", "full VolumeMode string");
    expect(shareaudio::to_string(shareaudio::VolumeMode::System) == "system", "system VolumeMode string");
    expect(shareaudio::parse_volume_mode("FULL") == shareaudio::VolumeMode::Full, "parse full VolumeMode case-insensitively");
    expect(shareaudio::parse_volume_mode("System") == shareaudio::VolumeMode::System, "parse system VolumeMode case-insensitively");
    expect(!shareaudio::parse_volume_mode("automatic").has_value(), "invalid VolumeMode is rejected");

    config.audio.channels = 1;
    expect(!shareaudio::validate(config).ok(), "mono config is rejected");

    const auto path = std::filesystem::temp_directory_path() / "shareaudio-config-test.json";
    shareaudio::AppConfig saved;
    saved.receiver.host = "192.168.1.55";
    saved.transmitter.mode = shareaudio::AudioMode::Fast;
    saved.transmitter.volume_mode = shareaudio::VolumeMode::System;
    saved.receiver.mode = shareaudio::AudioMode::Fast;
    expect(shareaudio::save_config_file(path, saved).ok(), "config saves to JSON");
    auto loaded = shareaudio::load_config_file(path);
    expect(loaded.ok(), "config loads from JSON");
    if (loaded.ok()) {
        expect(loaded.value().receiver.host == "192.168.1.55", "loaded config preserves receiver host");
        expect(loaded.value().transmitter.mode == shareaudio::AudioMode::Fast, "loaded config preserves mode");
        expect(loaded.value().transmitter.volume_mode == shareaudio::VolumeMode::System, "loaded config preserves VolumeMode");
    }
    std::filesystem::remove(path);

    const auto legacy_port_path = std::filesystem::temp_directory_path() / "shareaudio-legacy-port-test.json";
    {
        std::ofstream out(legacy_port_path);
        out << "{\n"
            << "  \"transmitter\": { \"port\": 8080 },\n"
            << "  \"receiver\": { \"port\": 8080 }\n"
            << "}\n";
    }
    auto legacy_port_loaded = shareaudio::load_config_file(legacy_port_path);
    expect(legacy_port_loaded.ok(), "legacy port config loads");
    if (legacy_port_loaded.ok()) {
        expect(legacy_port_loaded.value().transmitter.network.port == shareaudio::Defaults::tcp_port, "legacy transmitter port migrates to default TCP port");
        expect(legacy_port_loaded.value().receiver.port == shareaudio::Defaults::tcp_port, "legacy receiver port migrates to default TCP port");
    }
    std::filesystem::remove(legacy_port_path);

    const auto custom_port_path = std::filesystem::temp_directory_path() / "shareaudio-custom-port-test.json";
    {
        std::ofstream out(custom_port_path);
        out << "{\n"
            << "  \"transmitter\": { \"port\": 39095 },\n"
            << "  \"receiver\": { \"port\": 39095 }\n"
            << "}\n";
    }
    auto custom_port_loaded = shareaudio::load_config_file(custom_port_path);
    expect(custom_port_loaded.ok(), "custom port config loads");
    if (custom_port_loaded.ok()) {
        expect(custom_port_loaded.value().transmitter.network.port == 39095, "custom transmitter port is preserved");
        expect(custom_port_loaded.value().receiver.port == 39095, "custom receiver port is preserved");
    }
    std::filesystem::remove(custom_port_path);
}

void test_protocol()
{
    std::vector<std::uint8_t> balanced(2048, 1);
    std::vector<std::uint8_t> fast(1024, 2);
    std::vector<std::uint8_t> wrong(7, 3);

    expect(shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Balanced, balanced).ok(), "balanced PCM packet accepted");
    expect(shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Fast, fast).ok(), "fast PCM packet accepted");
    expect(!shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Balanced, wrong).ok(), "wrong PCM packet rejected");
    expect(!shareaudio::ProtocolWriter::validate_pcm_packet(shareaudio::AudioMode::Efficient, std::vector<std::uint8_t>(3840)).ok(), "efficient is rejected as raw PCM packet");

    auto header = shareaudio::ProtocolWriter::encode_opus_length(513);
    expect(header[0] == 0x02 && header[1] == 0x01, "opus length is big endian");
    expect(shareaudio::ProtocolReader::decode_opus_length(header).value() == 513, "opus length decodes");
    expect(!shareaudio::ProtocolReader::decode_opus_length(std::vector<std::uint8_t> { 1 }).ok(), "short opus header rejected");
    expect(!shareaudio::ProtocolReader::validate_opus_frame_length(0).ok(), "zero opus frame length rejected");

    std::vector<std::uint8_t> opus_frame(20, 9);
    auto packet = shareaudio::ProtocolWriter::make_opus_packet(opus_frame);
    expect(packet.ok(), "opus packet is created");
    expect(packet.value().size() == 22, "opus packet includes 2 byte header");

    auto balanced_header = shareaudio::ProtocolWriter::make_stream_header(shareaudio::AudioMode::Balanced);
    expect(balanced_header.ok(), "balanced stream header is created");
    if (balanced_header.ok()) {
        expect(balanced_header.value()[5] == 0x01, "balanced SAL1 mode id stays 1");
    }

    auto stream_header = shareaudio::ProtocolWriter::make_stream_header(shareaudio::AudioMode::Fast);
    expect(stream_header.ok(), "stream header is created");
    if (stream_header.ok()) {
        expect(stream_header.value()[5] == 0x02, "fast SAL1 mode id stays 2");
    }
    auto parsed_header = shareaudio::ProtocolReader::parse_stream_header(stream_header.value());
    expect(parsed_header.ok(), "stream header parses");
    if (parsed_header.ok()) {
        expect(parsed_header.value().mode == shareaudio::AudioMode::Fast, "stream header mode is fast");
        expect(parsed_header.value().codec == shareaudio::StreamCodec::PcmS16Le, "stream header codec is pcm");
        expect(parsed_header.value().sample_rate == 48000, "stream header sample rate is preserved");
        expect(parsed_header.value().packet_size == 1024, "stream header packet size is preserved");
    }

    auto invalid_magic = stream_header.value();
    invalid_magic[0] = 'X';
    expect(!shareaudio::ProtocolReader::parse_stream_header(invalid_magic).ok(), "stream header rejects invalid magic");

    auto invalid_packet_size = stream_header.value();
    invalid_packet_size[9] = 0;
    invalid_packet_size[10] = 9;
    expect(!shareaudio::ProtocolReader::parse_stream_header(invalid_packet_size).ok(), "stream header rejects mismatched packet size");

    auto efficient_header = shareaudio::ProtocolWriter::make_stream_header(shareaudio::AudioMode::Efficient);
    expect(efficient_header.ok(), "efficient stream header is created");
    if (efficient_header.ok()) {
        expect(efficient_header.value()[5] == 0x03, "efficient SAL1 mode id stays 3");
        auto parsed_efficient = shareaudio::ProtocolReader::parse_stream_header(efficient_header.value());
        expect(parsed_efficient.ok(), "efficient stream header parses");
        if (parsed_efficient.ok()) {
            expect(parsed_efficient.value().mode == shareaudio::AudioMode::Efficient, "efficient stream header mode is efficient");
            expect(parsed_efficient.value().codec == shareaudio::StreamCodec::Opus, "efficient stream header codec is opus");
            expect(parsed_efficient.value().packet_size == shareaudio::Defaults::max_opus_frame_bytes, "efficient stream header carries max opus frame bytes");
        }
    }
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

void test_pcm_chunker()
{
    shareaudio::PcmChunker chunker(shareaudio::AudioMode::Fast);
    expect(chunker.packet_size() == 1024, "fast chunker packet size");
    chunker.push(std::vector<std::uint8_t>(1000, 1));
    expect(!chunker.has_packet(), "chunker waits for a full packet");
    chunker.push(std::vector<std::uint8_t>(48, 2));
    expect(chunker.has_packet(), "chunker has full packet after enough bytes");
    auto packet = chunker.pop_packet();
    expect(packet.size() == 1024, "chunker emits exact packet size");
    expect(chunker.buffered_bytes() == 24, "chunker keeps remainder bytes");
    chunker.reset();
    expect(chunker.buffered_bytes() == 0, "chunker reset clears bytes");

    shareaudio::PcmChunker opus_chunker(shareaudio::Defaults::opus_pcm_frame_bytes);
    expect(opus_chunker.packet_size() == 3840, "opus chunker packet size is 20 ms PCM");
    opus_chunker.push(std::vector<std::uint8_t>(3839, 1));
    expect(!opus_chunker.has_packet(), "opus chunker waits for a full 20 ms frame");
    opus_chunker.push(std::vector<std::uint8_t>(1, 2));
    expect(opus_chunker.has_packet(), "opus chunker has full 20 ms frame");
    auto opus_pcm = opus_chunker.pop_packet();
    expect(opus_pcm.size() == shareaudio::Defaults::opus_pcm_frame_bytes, "opus chunker emits exact PCM frame size");
}

void test_pcm_pipelines()
{
    shareaudio::PcmTransmitterPipeline transmitter(shareaudio::AudioMode::Balanced, 2);
    transmitter.on_captured_pcm(std::vector<std::uint8_t>(4096, 7));
    auto stats = transmitter.stats();
    expect(stats.bytes_captured == 4096, "transmitter pipeline tracks captured bytes");
    expect(stats.packets_produced == 2, "transmitter pipeline produces balanced packets");
    expect(stats.queued_packets == 2, "transmitter pipeline queues packets");

    std::vector<std::uint8_t> packet;
    expect(transmitter.try_pop_packet(packet), "transmitter pipeline pops first packet");
    expect(packet.size() == 2048, "transmitter packet is balanced size");

    const auto unchanged_pcm = constant_stereo_pcm(256, -32768);
    shareaudio::PcmTransmitterPipeline full_volume(shareaudio::AudioMode::Fast, 2, shareaudio::VolumeMode::Full);
    full_volume.set_volume_gain(0.0f);
    full_volume.on_captured_pcm(unchanged_pcm);
    std::vector<std::uint8_t> unchanged_packet;
    expect(full_volume.try_pop_packet(unchanged_packet), "full VolumeMode emits PCM");
    expect(unchanged_packet == unchanged_pcm, "full VolumeMode preserves PCM bytes exactly");

    shareaudio::PcmTransmitterPipeline runtime_volume(shareaudio::AudioMode::Fast, 4, shareaudio::VolumeMode::System);
    runtime_volume.set_volume_gain(0.0f);
    runtime_volume.on_captured_pcm(constant_stereo_pcm(512, 10000));
    std::vector<std::uint8_t> runtime_ramp_packet;
    std::vector<std::uint8_t> runtime_muted_packet;
    expect(runtime_volume.try_pop_packet(runtime_ramp_packet), "runtime system VolumeMode emits ramped PCM");
    expect(runtime_volume.try_pop_packet(runtime_muted_packet), "runtime system VolumeMode emits muted PCM");
    runtime_volume.set_volume_mode(shareaudio::VolumeMode::Full);
    runtime_volume.on_captured_pcm(unchanged_pcm);
    expect(runtime_volume.try_pop_packet(unchanged_packet), "runtime full VolumeMode emits PCM");
    expect(unchanged_packet == unchanged_pcm, "runtime full VolumeMode immediately bypasses gain processing");

    shareaudio::PcmTransmitterPipeline muted_volume(shareaudio::AudioMode::Balanced, 4, shareaudio::VolumeMode::System);
    muted_volume.set_volume_gain(0.0f);
    muted_volume.on_captured_pcm(constant_stereo_pcm(1024, 10000));
    std::vector<std::uint8_t> ramp_packet;
    std::vector<std::uint8_t> muted_packet;
    expect(muted_volume.try_pop_packet(ramp_packet), "system VolumeMode emits ramp packet");
    expect(muted_volume.try_pop_packet(muted_packet), "system VolumeMode emits post-ramp packet");
    expect(pcm_sample_at(ramp_packet, 0) == 10000, "volume ramp starts at the previous gain");
    expect(std::abs(pcm_sample_at(ramp_packet, 240 * 4)) > 4500
        && std::abs(pcm_sample_at(ramp_packet, 240 * 4)) < 5500, "volume ramp reaches approximately half gain at 5 ms");
    expect(std::all_of(muted_packet.begin(), muted_packet.end(), [](std::uint8_t byte) { return byte == 0; }),
        "system VolumeMode reaches digital silence after the 10 ms ramp");

    shareaudio::PcmTransmitterPipeline half_volume(shareaudio::AudioMode::Balanced, 4, shareaudio::VolumeMode::System);
    half_volume.set_volume_gain(0.5f);
    half_volume.on_captured_pcm(constant_stereo_pcm(1024, 10000));
    std::vector<std::uint8_t> half_ramp_packet;
    std::vector<std::uint8_t> half_packet;
    expect(half_volume.try_pop_packet(half_ramp_packet), "half gain ramp packet is available");
    expect(half_volume.try_pop_packet(half_packet), "half gain settled packet is available");
    expect(pcm_sample_at(half_packet, 0) == 5000, "settled system VolumeMode scales positive PCM samples");
    expect(pcm_sample_at(half_packet, 2) == 5000, "system VolumeMode applies equal gain to both channels");

    shareaudio::NullAudioPlayback playback;
    shareaudio::AudioFormat format;
    expect(playback.initialize(format, "null").ok(), "pipeline playback initializes");
    shareaudio::PcmReceiverPipeline receiver(playback, 4096);
    expect(receiver.start().ok(), "receiver pipeline starts playback");
    expect(receiver.receive_pcm(std::vector<std::uint8_t>(2048, 3)).ok(), "receiver pipeline accepts PCM");
    expect(receiver.pump_playback(1024).ok(), "receiver pipeline pumps playback");
    auto receiver_stats = receiver.stats();
    expect(receiver_stats.bytes_received == 2048, "receiver tracks received bytes");
    expect(receiver_stats.bytes_played == 1024, "receiver tracks played bytes");

    shareaudio::PcmTransmitterPipeline efficient_transmitter(shareaudio::AudioMode::Efficient, 2);
    std::vector<std::uint8_t> opus_pcm_in(shareaudio::Defaults::opus_pcm_frame_bytes);
    for (std::size_t i = 0; i < opus_pcm_in.size() / 2; ++i) {
        std::int16_t sample = static_cast<std::int16_t>(1000.0 * std::sin(2.0 * 3.14159 * 440.0 * i / 48000.0));
        std::memcpy(&opus_pcm_in[i * 2], &sample, sizeof(sample));
    }
    efficient_transmitter.on_captured_pcm(opus_pcm_in);
    auto efficient_stats = efficient_transmitter.stats();
#if SHAREAUDIO_HAS_LIBOPUS
    expect(efficient_stats.packets_produced == 1, "efficient transmitter produces one opus packet from 20 ms PCM");
    std::vector<std::uint8_t> opus_packet;
    expect(efficient_transmitter.try_pop_packet(opus_packet), "efficient transmitter pops opus packet");
    expect(opus_packet.size() > 2, "efficient opus packet includes length and payload");
    if (opus_packet.size() > 2) {
        auto opus_len = shareaudio::ProtocolReader::decode_opus_length(std::span<const std::uint8_t>(opus_packet.data(), 2));
        expect(opus_len.ok(), "efficient opus packet length decodes");
        if (opus_len.ok()) {
            expect(static_cast<std::size_t>(opus_len.value()) + 2 == opus_packet.size(), "efficient opus packet length matches payload");
        }
    }

    shareaudio::PcmTransmitterPipeline muted_efficient(
        shareaudio::AudioMode::Efficient, 4, shareaudio::VolumeMode::System);
    muted_efficient.set_volume_gain(0.0f);
    muted_efficient.on_captured_pcm(constant_stereo_pcm(1920, 12000));
    std::vector<std::uint8_t> ramp_opus_packet;
    std::vector<std::uint8_t> silent_opus_packet;
    expect(muted_efficient.try_pop_packet(ramp_opus_packet), "efficient VolumeMode emits ramped Opus packet");
    expect(muted_efficient.try_pop_packet(silent_opus_packet), "efficient VolumeMode emits settled Opus packet");
    expect(ramp_opus_packet.size() > 2 && silent_opus_packet.size() > 2,
        "efficient pipeline keeps valid Opus framing with System VolumeMode");
#else
    expect(efficient_stats.packets_produced == 0, "efficient transmitter produces no packets without libopus");
    expect(efficient_stats.dropped_packets == 1, "efficient transmitter drops unsupported opus frame without libopus");
#endif
}

void test_system_volume_math()
{
    expect(std::abs(shareaudio::gain_from_decibels(-6.0f, false) - 0.501187f) < 0.0001f,
        "minus 6 dB converts to the expected linear PCM gain");
    expect(shareaudio::gain_from_decibels(-20.0f, true) == 0.0f, "mute overrides the endpoint dB level");
    expect(shareaudio::gain_from_decibels(6.0f, false) == 1.0f, "positive endpoint gain is clamped to prevent amplification");
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
    expect(controller.set_audio_mode(shareaudio::AudioMode::Fast).ok(), "mode can be set while idle");
    expect(controller.start_transmitter().ok(), "transmitter can start");
    expect(!controller.connect_receiver("192.168.1.20").ok(), "receiver cannot start while transmitter active");
    expect(controller.stop_transmitter().ok(), "transmitter can stop");
    expect(!controller.connect_receiver("127.0.0.1").ok(), "self connection is blocked");
}

void test_session_controller()
{
    shareaudio::AppConfig config;
    config.transmitter.network.port = 0;
    config.receiver.port = 0;
    shareaudio::SessionControllerOptions options;
    options.backend = shareaudio::SessionAudioBackend::Fake;
    options.recent_devices_path = std::filesystem::temp_directory_path() / "shareaudio-session-recent-test.json";
    shareaudio::SessionController session(config, options);

#ifdef _WIN32
    expect(session.set_local_audio_muted(true).ok(), "local audio mute can be armed while idle");
    auto armed_mute_status = session.status_snapshot();
    expect(armed_mute_status.mute_local_audio_requested, "idle local audio mute request is reported");
    expect(!armed_mute_status.local_audio_muted, "idle local audio remains enabled");
    expect(session.set_volume_mode(shareaudio::VolumeMode::System).ok(), "system volume can be selected while idle");
    expect(session.set_local_audio_muted(true).ok(), "local mute overrides system volume while idle");
    expect(session.config_snapshot().transmitter.volume_mode == shareaudio::VolumeMode::Full,
        "local mute forces full VolumeMode");
    expect(session.set_volume_mode(shareaudio::VolumeMode::System).ok(), "system volume can replace local mute");
    expect(!session.status_snapshot().mute_local_audio_requested,
        "system volume clears the local mute request");
    expect(session.config_snapshot().transmitter.volume_mode == shareaudio::VolumeMode::System,
        "system volume remains selected after clearing local mute");
    expect(session.set_local_audio_muted(false).ok(), "idle local audio mute can be disarmed");
#else
    expect(!session.set_local_audio_muted(true).ok(), "local audio mute is rejected outside Windows");
#endif

#if SHAREAUDIO_HAS_LIBOPUS
    expect(session.start_sharing(shareaudio::AudioMode::Efficient).ok(), "session accepts efficient sharing");
    expect(session.stop_sharing().ok(), "session stops efficient sharing");
#else
    expect(!session.start_sharing(shareaudio::AudioMode::Efficient).ok(), "session rejects efficient sharing without libopus");
#endif
    expect(session.start_sharing(shareaudio::AudioMode::Balanced, {}, shareaudio::VolumeMode::System).ok(), "session starts fake sharing");
    expect(session.set_volume_mode(shareaudio::VolumeMode::Full).ok(), "session switches to full VolumeMode while sharing");
    auto full_status = session.status_snapshot();
    expect(full_status.sharing_active, "runtime VolumeMode change keeps sharing active");
    expect(full_status.volume_mode == shareaudio::VolumeMode::Full, "runtime full VolumeMode is reported");
    expect(full_status.system_volume_gain == 1.0f, "runtime full VolumeMode resets gain");
    expect(full_status.system_volume_tracking == shareaudio::SystemVolumeTrackingState::Disabled,
        "runtime full VolumeMode disables tracking");
    expect(session.set_volume_mode(shareaudio::VolumeMode::System).ok(), "session switches to system VolumeMode while sharing");
    auto system_status = session.status_snapshot();
    expect(system_status.sharing_active, "runtime system VolumeMode keeps sharing active");
    expect(system_status.volume_mode == shareaudio::VolumeMode::System, "runtime system VolumeMode is reported");
    expect(session.start_listening("192.168.1.50").ok(), "session starts receiver while sharing");
    auto sharing_status = session.status_snapshot();
    expect(sharing_status.sharing_active, "session status reports active sharing");
    expect(sharing_status.volume_mode == shareaudio::VolumeMode::System, "session status reports system VolumeMode");
#ifdef _WIN32
    expect(sharing_status.system_volume_tracking == shareaudio::SystemVolumeTrackingState::Fallback,
        "fake Windows capture reports system volume fallback");
#else
    expect(sharing_status.system_volume_tracking == shareaudio::SystemVolumeTrackingState::Unsupported,
        "non-Windows system VolumeMode reports unsupported");
#endif
    expect(sharing_status.receiver_connecting, "session status reports receiver connecting during sharing");
    expect(sharing_status.mode == shareaudio::SessionMode::SharingConnecting, "session status is sharing and connecting");
    expect(session.stop_sharing().ok(), "session stops fake sharing without stopping receiver");
    auto receiver_status = session.status_snapshot();
    expect(!receiver_status.sharing_active, "sharing stop leaves sharing inactive");
    expect(receiver_status.receiver_connecting, "sharing stop keeps receiver active");
    expect(session.stop_listening().ok(), "session stops receiver after sharing stop");
    expect(session.stop_sharing().ok(), "session sharing stop is idempotent");

    shareaudio::SessionController self_blocking(config, options);
    expect(!self_blocking.start_listening("127.0.0.1").ok(), "session blocks self connection");
}

void test_session_controller_listen_loopback()
{
    constexpr std::uint16_t port = 39093;

    shareaudio::PcmBroadcastServer server;
    auto started = server.start(port, shareaudio::AudioMode::Balanced);
    expect(started.ok(), started.ok() ? "session loopback server started" : started.error().message);
    if (!started.ok()) {
        return;
    }

    shareaudio::AppConfig config;
    config.receiver.port = port;
    config.transmitter.network.port = 0;
    shareaudio::SessionControllerOptions options;
    options.backend = shareaudio::SessionAudioBackend::Fake;
    options.allow_self_connection = true;
    options.recent_devices_path = std::filesystem::temp_directory_path() / "shareaudio-session-loopback-recent-test.json";
    shareaudio::SessionController session(config, options);
    expect(session.start_listening("127.0.0.1").ok(), "session starts loopback listening");

    bool listening = false;
    for (int i = 0; i < 50; ++i) {
        if (session.status_snapshot().mode == shareaudio::SessionMode::Listening) {
            listening = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    expect(listening, "session reaches listening state after SAL1 header");
    auto status = session.status_snapshot();
    expect(status.has_detected_mode && status.detected_mode == shareaudio::AudioMode::Balanced, "session autodetects balanced mode");
    expect(session.start_sharing(shareaudio::AudioMode::Balanced).ok(), "session starts sharing while listening");
    auto combined_status = session.status_snapshot();
    expect(combined_status.sharing_active, "combined status reports sharing active");
    expect(combined_status.receiver_listening, "combined status reports receiver listening");
    expect(combined_status.mode == shareaudio::SessionMode::SharingListening, "combined status is sharing and listening");
    expect(session.stop_sharing().ok(), "combined session stops sharing only");
    auto listen_only_status = session.status_snapshot();
    expect(!listen_only_status.sharing_active, "sharing stop leaves receiver-only session");
    expect(listen_only_status.receiver_listening, "receiver remains listening after sharing stop");
    expect(session.stop_listening().ok(), "session stops loopback listening");
    expect(session.stop_listening().ok(), "session listening stop is idempotent");
    server.stop();
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

void test_opus_codec()
{
    shareaudio::OpusEncoder encoder;
    shareaudio::OpusDecoder decoder;
    shareaudio::AudioFormat format; // 48kHz, stereo

    auto init_enc = encoder.initialize(format, shareaudio::Defaults::opus_bitrate_bps);
    auto init_dec = decoder.initialize(format);

#if SHAREAUDIO_HAS_LIBOPUS
    expect(init_enc.ok(), "opus encoder initialization succeeds");
    expect(init_dec.ok(), "opus decoder initialization succeeds");

    if (init_enc.ok() && init_dec.ok()) {
        // Create simple 20ms sine wave or dummy data: 48000 Hz * 0.02s = 960 frames.
        // 960 frames * 2 channels * 2 bytes/sample = 3840 bytes.
        std::vector<std::uint8_t> pcm_in(3840);
        for (std::size_t i = 0; i < pcm_in.size() / 2; ++i) {
            std::int16_t sample = static_cast<std::int16_t>(1000.0 * sin(2.0 * 3.14159 * 440.0 * i / 48000.0));
            std::memcpy(&pcm_in[i * 2], &sample, sizeof(sample));
        }

        auto encoded = encoder.encode(pcm_in);
        expect(encoded.ok(), "opus encode succeeds");
        if (encoded.ok()) {
            expect(!encoded.value().empty(), "encoded opus packet is not empty");
            expect(encoded.value().size() < pcm_in.size(), "encoded opus packet is compressed (smaller than raw pcm)");

            auto decoded = decoder.decode(encoded.value());
            expect(decoded.ok(), "opus decode succeeds");
            if (decoded.ok()) {
                expect(decoded.value().size() == pcm_in.size(), "decoded pcm size matches input size");
            }
        }
    }
#else
    expect(init_enc.error().code == shareaudio::ErrorCode::NotSupported, "opus encoder reports not supported without libopus");
    expect(init_dec.error().code == shareaudio::ErrorCode::NotSupported, "opus decoder reports not supported without libopus");
#endif
}

void test_tcp_loopback()
{
    auto result = shareaudio::run_loopback_tcp_self_test();
    expect(result.ok(), result.ok() ? "loopback TCP self-test passed" : result.error().message);
}

void test_pcm_broadcast_server()
{
    constexpr std::uint16_t port = 39092;
    std::vector<std::uint8_t> packet(128, 42);

    shareaudio::PcmBroadcastServer server;
    auto started = server.start(port, shareaudio::AudioMode::Balanced);
    expect(started.ok(), started.ok() ? "broadcast server started" : started.error().message);
    if (!started.ok()) {
        return;
    }

    auto client = shareaudio::TcpSocket::connect_to("127.0.0.1", port);
    expect(client.ok(), client.ok() ? "broadcast client connected" : client.error().message);
    if (!client.ok()) {
        server.stop();
        return;
    }

    for (int i = 0; i < 20 && server.stats().connected_clients == 0; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto received_header = client.value().receive_exact(shareaudio::ProtocolWriter::stream_header_size);
    expect(received_header.ok(), received_header.ok() ? "broadcast client received stream header" : received_header.error().message);
    if (received_header.ok()) {
        auto parsed_header = shareaudio::ProtocolReader::parse_stream_header(received_header.value());
        expect(parsed_header.ok(), parsed_header.ok() ? "broadcast stream header parsed" : parsed_header.error().message);
        if (parsed_header.ok()) {
            expect(parsed_header.value().mode == shareaudio::AudioMode::Balanced, "broadcast stream header mode is balanced");
        }
    }

    server.broadcast(packet);
    auto received = client.value().receive_exact(packet.size());
    expect(received.ok(), received.ok() ? "broadcast client received packet after header" : received.error().message);
    if (received.ok()) {
        expect(received.value() == packet, "broadcast packet content matches");
    }
    expect(server.stats().bytes_sent >= packet.size(), "broadcast server tracks bytes sent");
    server.stop();
}

void test_hybrid_broadcast_server()
{
    constexpr std::uint16_t port = 39093;
    shareaudio::PcmBroadcastServer server;
    auto started = server.start(port, shareaudio::AudioMode::Balanced);
    expect(started.ok(), started.ok() ? "hybrid server started" : started.error().message);
    if (!started.ok()) {
        return;
    }

    auto client_info = shareaudio::TcpSocket::connect_to("127.0.0.1", port);
    expect(client_info.ok(), "info client connected");
    if (client_info.ok()) {
        std::string req = "GET /info HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
        auto sent = client_info.value().send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(req.data()), req.size()));
        expect(sent.ok(), "info request sent");

        std::string res;
        while (client_info.value().valid() && res.size() < 2048) {
            auto chunk = client_info.value().receive_exact(1);
            if (!chunk.ok() || chunk.value().empty()) {
                break;
            }
            res.push_back(static_cast<char>(chunk.value()[0]));
        }
        expect(res.find("HTTP/1.1 200 OK") != std::string::npos, "info response is HTTP 200");
        expect(res.find("\"codec\": \"pcm\"") != std::string::npos, "info JSON contains correct codec");
        expect(res.find("\"chunkSize\": 2048") != std::string::npos, "info JSON contains correct chunkSize");
    }

    auto client_page = shareaudio::TcpSocket::connect_to("127.0.0.1", port);
    expect(client_page.ok(), "web receiver page client connected");
    if (client_page.ok()) {
        std::string req = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
        auto sent = client_page.value().send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(req.data()), req.size()));
        expect(sent.ok(), "web receiver page request sent");

        std::string res;
        while (client_page.value().valid() && res.size() < 20000) {
            auto chunk = client_page.value().receive_exact(1);
            if (!chunk.ok() || chunk.value().empty()) {
                break;
            }
            res.push_back(static_cast<char>(chunk.value()[0]));
        }
        expect(res.find("ShareAudioPC<br>Web receiver") != std::string::npos, "root page serves the branded web receiver");
        expect(res.find("fetch('/info'") != std::string::npos, "web receiver page queries /info");
        expect(res.find("fetch('/stream'") != std::string::npos, "web receiver page opens /stream");
        expect(res.find("Disconnect") != std::string::npos, "web receiver page includes disconnect button state");
        expect(res.find("abortController.abort()") != std::string::npos, "web receiver page can abort the stream");
        expect(res.find("renderBlockBytes: 2048") != std::string::npos, "web receiver page includes Fast render block");
        expect(res.find("initialTarget: 0.040") != std::string::npos, "web receiver page includes Fast adaptive target");
        expect(res.find("renderBlockBytes: 4096") != std::string::npos, "web receiver page includes Balanced render block");
        expect(res.find("maxAhead: 0.320") != std::string::npos, "web receiver page includes Balanced max scheduled buffer");
        expect(res.find("prebufferBytes()") != std::string::npos, "web receiver page prebuffers before playback");
        expect(res.find("maybeAdaptTarget") != std::string::npos, "web receiver page adapts jitter target");
        expect(res.find("This web receiver supports only Fast and Balanced PCM") != std::string::npos, "web receiver page rejects non-PCM modes");
    }

    auto client_stream = shareaudio::TcpSocket::connect_to("127.0.0.1", port);
    expect(client_stream.ok(), "stream client connected");
    if (client_stream.ok()) {
        std::string req = "GET /stream HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: keep-alive\r\n\r\n";
        auto sent = client_stream.value().send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(req.data()), req.size()));
        expect(sent.ok(), "stream request sent");

        for (int i = 0; i < 20 && server.stats().connected_clients == 0; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        std::string headers;
        while (client_stream.value().valid() && headers.size() < 2048) {
            auto chunk = client_stream.value().receive_exact(1);
            if (!chunk.ok() || chunk.value().empty()) {
                break;
            }
            headers.push_back(static_cast<char>(chunk.value()[0]));
            if (headers.size() >= 4 && headers.substr(headers.size() - 4) == "\r\n\r\n") {
                break;
            }
        }
        expect(headers.find("HTTP/1.1 200 OK") != std::string::npos, "stream response headers start with 200 OK");
        expect(headers.find("Content-Type: application/octet-stream") != std::string::npos, "stream content type is octet-stream");
        expect(headers.find("Content-Length") == std::string::npos, "stream response has no content length");
        expect(headers.find("Transfer-Encoding") == std::string::npos, "stream response does not use transfer encoding");

        std::vector<std::uint8_t> test_packet(64, 99);
        server.broadcast(test_packet);

        auto received = client_stream.value().receive_exact(64);
        expect(received.ok(), "received packet bytes after HTTP headers");
        if (received.ok()) {
            expect(received.value() == test_packet, "received packet content matches sent content exactly");
        }
    }

    const auto clients_before_native = server.stats().connected_clients;
    auto client_native = shareaudio::TcpSocket::connect_to("127.0.0.1", port);
    expect(client_native.ok(), "native client connected");
    if (client_native.ok()) {
        auto header_bytes = client_native.value().receive_exact(shareaudio::ProtocolWriter::stream_header_size);
        expect(header_bytes.ok(), "native client received SAL1 header after timeout");
        if (header_bytes.ok()) {
            auto parsed = shareaudio::ProtocolReader::parse_stream_header(header_bytes.value());
            expect(parsed.ok(), "parsed SAL1 header successfully");
            if (parsed.ok()) {
                expect(parsed.value().mode == shareaudio::AudioMode::Balanced, "SAL1 mode matches server mode");
            }
        }

        for (int i = 0; i < 100 && server.stats().connected_clients < clients_before_native + 1; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        expect(server.stats().connected_clients >= clients_before_native + 1, "native client registered before broadcast");

        auto slow_stream = shareaudio::TcpSocket::connect_to("127.0.0.1", port);
        expect(slow_stream.ok(), "slow HTTP stream client connected");
        if (slow_stream.ok()) {
            std::string req = "GET /stream HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: keep-alive\r\n\r\n";
            auto sent = slow_stream.value().send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(req.data()), req.size()));
            expect(sent.ok(), "slow stream request sent");

            std::string headers;
            while (slow_stream.value().valid() && headers.size() < 2048) {
                auto chunk = slow_stream.value().receive_exact(1);
                if (!chunk.ok() || chunk.value().empty()) {
                    break;
                }
                headers.push_back(static_cast<char>(chunk.value()[0]));
                if (headers.size() >= 4 && headers.substr(headers.size() - 4) == "\r\n\r\n") {
                    break;
                }
            }
            expect(headers.find("HTTP/1.1 200 OK") != std::string::npos, "slow stream response headers start with 200 OK");
        }

        std::vector<std::uint8_t> native_packet(64, 7);
        server.broadcast(native_packet);
        auto received = client_native.value().receive_with_timeout(native_packet.size(), 1000);
        expect(received.ok(), "native client receives packet while HTTP client is slow");
        if (received.ok()) {
            expect(received.value() == native_packet, "native packet content is preserved with slow HTTP client");
        }
    }

    server.stop();
}

void test_session_controller_http_opus_fallback()
{
#if SHAREAUDIO_HAS_LIBOPUS
    constexpr std::uint16_t port = 39094;

    shareaudio::AudioFormat format;
    shareaudio::OpusEncoder encoder;
    auto initialized = encoder.initialize(format, shareaudio::Defaults::opus_bitrate_bps);
    expect(initialized.ok(), initialized.ok() ? "HTTP Opus fallback test encoder initialized" : initialized.error().message);
    if (!initialized.ok()) {
        return;
    }

    std::vector<std::uint8_t> pcm_in(3840);
    for (std::size_t i = 0; i < pcm_in.size() / 2; ++i) {
        std::int16_t sample = static_cast<std::int16_t>(1000.0 * sin(2.0 * 3.14159 * 440.0 * i / 48000.0));
        std::memcpy(&pcm_in[i * 2], &sample, sizeof(sample));
    }

    auto encoded = encoder.encode(pcm_in);
    expect(encoded.ok(), encoded.ok() ? "HTTP Opus fallback test frame encoded" : encoded.error().message);
    if (!encoded.ok()) {
        return;
    }

    auto packet = shareaudio::ProtocolWriter::make_opus_packet(encoded.value());
    expect(packet.ok(), packet.ok() ? "HTTP Opus fallback test packet framed" : packet.error().message);
    if (!packet.ok()) {
        return;
    }

    shareaudio::TcpTransmitterServer server;
    auto started = server.start(port, [opus_packet = packet.value()](std::shared_ptr<shareaudio::TcpSocket> client) {
        auto first_bytes = client->receive_with_timeout(4, 200);
        if (!first_bytes.ok() || first_bytes.value().size() != 4) {
            client->close();
            return;
        }

        std::string request(first_bytes.value().begin(), first_bytes.value().end());
        if (request != "GET ") {
            client->close();
            return;
        }

        bool found_separator = false;
        while (client->valid() && request.size() < 4096) {
            auto byte = client->receive_with_timeout(1, 1000);
            if (!byte.ok() || byte.value().empty()) {
                break;
            }
            request.push_back(static_cast<char>(byte.value()[0]));
            if (request.size() >= 4 && request.substr(request.size() - 4) == "\r\n\r\n") {
                found_separator = true;
                break;
            }
        }

        if (!found_separator) {
            client->close();
            return;
        }

        std::string path = "/";
        auto space1 = request.find(' ');
        if (space1 != std::string::npos) {
            auto space2 = request.find(' ', space1 + 1);
            if (space2 != std::string::npos) {
                path = request.substr(space1 + 1, space2 - (space1 + 1));
            }
        }

        if (path == "/info") {
            std::string body = "{\n"
                "  \"status\": \"streaming\",\n"
                "  \"connectedClients\": 0,\n"
                "  \"sampleRate\": 48000,\n"
                "  \"channels\": 2,\n"
                "  \"codec\": \"opus\",\n"
                "  \"bitrate\": 128000,\n"
                "  \"chunkSize\": 4096\n"
                "}\n";
            std::string response = "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/json\r\n"
                "Connection: close\r\n"
                "Content-Length: " + std::to_string(body.size()) + "\r\n"
                "\r\n" + body;
            (void)client->send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(response.data()), response.size()));
            client->close();
            return;
        }

        if (path == "/stream") {
            std::string response = "HTTP/1.1 200 OK\r\n"
                "Content-Type: application/octet-stream\r\n"
                "Connection: keep-alive\r\n"
                "Cache-Control: no-cache, no-store, must-revalidate\r\n"
                "Pragma: no-cache\r\n"
                "\r\n";
            auto sent_headers = client->send_all(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(response.data()), response.size()));
            if (!sent_headers.ok()) {
                client->close();
                return;
            }

            for (int i = 0; i < 5 && client->valid(); ++i) {
                auto sent_packet = client->send_all(opus_packet);
                if (!sent_packet.ok()) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            client->close();
            return;
        }

        client->close();
    });
    expect(started.ok(), started.ok() ? "HTTP Opus fallback server started" : started.error().message);
    if (!started.ok()) {
        return;
    }

    shareaudio::AppConfig config;
    config.receiver.port = port;
    config.transmitter.network.port = port;
    shareaudio::SessionControllerOptions options;
    options.backend = shareaudio::SessionAudioBackend::Fake;
    options.allow_self_connection = true;
    options.recent_devices_path = std::filesystem::temp_directory_path() / "shareaudio-session-http-opus-recent-test.json";
    shareaudio::SessionController session(config, options);
    expect(session.start_listening("127.0.0.1").ok(), "session starts HTTP Opus fallback listening");

    bool detected_efficient = false;
    bool received_audio = false;
    for (int i = 0; i < 80; ++i) {
        const auto status = session.status_snapshot();
        detected_efficient = status.has_detected_mode && status.detected_mode == shareaudio::AudioMode::Efficient;
        received_audio = status.bytes_received > 0 && status.bytes_played > 0;
        if (detected_efficient && received_audio) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    const auto status = session.status_snapshot();
    expect(status.has_detected_mode && status.detected_mode == shareaudio::AudioMode::Efficient, "HTTP Opus fallback detects efficient AudioMode");
    expect(status.bytes_received > 0, "HTTP Opus fallback receives Opus bytes");
    expect(status.bytes_played > 0, "HTTP Opus fallback decodes PCM bytes");
    expect(session.stop_listening().ok(), "session stops HTTP Opus fallback listening");
    server.stop();
#endif
}

void test_console_commands()
{
    shareaudio::AppController controller;
    shareaudio::ConsoleUi ui(controller);

    expect(ui.run(std::vector<std::string> { "--status" }) == 2, "old --status command is removed");
    expect(ui.run(std::vector<std::string> { "--list-ips" }) == 2, "old --list-ips command is removed");
    expect(ui.run(std::vector<std::string> { "share", "--audio-mode", "invalid_mode" }) == 2, "invalid AudioMode is rejected");
    expect(ui.run(std::vector<std::string> { "share", "--volume-mode", "invalid_mode" }) == 2, "invalid VolumeMode is rejected");
    expect(ui.run(std::vector<std::string> { "share", "--mode", "quality" }) == 2, "old --mode flag is rejected");
    expect(ui.run(std::vector<std::string> { "listen" }) == 2, "listen requires a host");
    expect(ui.run(std::vector<std::string> { "help" }) == 0, "help command succeeds");
}

void test_single_instance()
{
    auto res1 = shareaudio::enforce_single_instance();
    expect(res1.ok(), "first single instance lock succeeds");

    auto res2 = shareaudio::enforce_single_instance();
    expect(res2.ok(), "second single instance lock with same PID succeeds");

    auto running = shareaudio::running_instance_pid();
    expect(running.ok() && running.value() != 0, "running instance exposes its PID");

    shareaudio::AppController controller;
    shareaudio::ConsoleUi ui(controller);
    expect(ui.run(std::vector<std::string> { "status" }) == 0, "status command succeeds");
    expect(shareaudio::running_instance_pid().value() == running.value(), "status does not stop the running instance");
}

void test_startup_config()
{
    namespace fs = std::filesystem;
    auto tmp_dir = fs::temp_directory_path() / "shareaudio-cfg-test";
    fs::create_directories(tmp_dir);

    // Test 1: Missing file returns default config
    {
        auto result = shareaudio::load_startup_config(tmp_dir / "nonexistent.cfg");
        expect(result.ok(), "missing cfg file returns success");
        expect(!result.value().autostart, "missing cfg has autostart=false");
        expect(!result.value().traymode, "missing cfg has traymode=false");
        expect(!result.value().startintray, "missing cfg has startintray=false");
        expect(!result.value().is_dark_theme(), "missing cfg defaults to light theme");
        expect(!result.value().mute_local_audio, "missing cfg defaults local audio mute to false");
        expect(result.value().mode.empty(), "missing cfg has empty mode");
        expect(!result.value().has_volume_mode(), "missing cfg has no VolumeMode override");
    }

    // Test 2: Full config file
    {
        auto cfg_path = tmp_dir / "full.cfg";
        std::ofstream out(cfg_path);
        out << "# ShareAudioLite test config\n";
        out << "AUTOSTART=true\n";
        out << "TRAYMODE=true\n";
        out << "STARTINTRAY=true\n";
        out << "MODE=server\n";
        out << "AUDIO_MODE=efficient\n";
        out << "THEME=dark\n";
        out << "VOLUME_MODE=system\n";
        out << "MUTE_LOCAL_AUDIO=true\n";
        out << "DEVICE_ID=my_capture_device\n";
        out << "PLAYBACK_DEVICE_ID=my_playback_device\n";
        out << "SERVER_IP=192.168.1.100\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "full cfg loads successfully");
        auto& cfg = result.value();
        expect(cfg.autostart, "full cfg autostart is true");
        expect(cfg.traymode, "full cfg traymode is true");
        expect(cfg.startintray, "full cfg startintray is true");
        expect(cfg.is_server(), "full cfg mode is server");
        expect(cfg.audio_mode == "efficient", "full cfg audio_mode is efficient");
        expect(cfg.volume_mode == "system", "full cfg volume_mode is system");
        expect(cfg.mute_local_audio, "full cfg enables local audio mute");
        expect(cfg.device_id == "my_capture_device", "full cfg device_id matches");
        expect(cfg.is_dark_theme(), "full cfg theme is dark");
        expect(cfg.playback_device_id == "my_playback_device", "full cfg playback_device_id matches");
        expect(cfg.server_ip == "192.168.1.100", "full cfg server_ip matches");
        expect(cfg.parsed_audio_mode().has_value(), "full cfg parsed_audio_mode is valid");
        expect(*cfg.parsed_audio_mode() == shareaudio::AudioMode::Efficient, "full cfg parsed_audio_mode is Efficient");
        expect(cfg.parsed_volume_mode() == shareaudio::VolumeMode::System, "full cfg parsed_volume_mode is System");
        fs::remove(cfg_path);
    }

    // Test 3: Partial config (only autostart and mode)
    {
        auto cfg_path = tmp_dir / "partial.cfg";
        std::ofstream out(cfg_path);
        out << "AUTOSTART=yes\n";
        out << "MODE=client\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "partial cfg loads successfully");
        auto& cfg = result.value();
        expect(cfg.autostart, "partial cfg autostart=yes is true");
        expect(!cfg.traymode, "partial cfg traymode defaults false");
        expect(!cfg.startintray, "partial cfg startintray defaults false");
        expect(cfg.is_client(), "partial cfg mode is client");
        expect(!cfg.has_audio_mode(), "partial cfg has no audio_mode");
        expect(!cfg.has_device_id(), "partial cfg has no device_id");
        expect(!cfg.has_server_ip(), "partial cfg has no server_ip");
        fs::remove(cfg_path);
    }

    // Test 4: Comments and blank lines are ignored
    {
        auto cfg_path = tmp_dir / "comments.cfg";
        std::ofstream out(cfg_path);
        out << "# This is a comment\n";
        out << "\n";
        out << "  # Another comment with leading whitespace\n";
        out << "AUTOSTART=false\n";
        out << "\n";
        out << "MODE=server\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "comments cfg loads successfully");
        expect(!result.value().autostart, "comments cfg autostart=false");
        expect(result.value().is_server(), "comments cfg mode parsed correctly");
        fs::remove(cfg_path);
    }

    // Test 5: Case-insensitive keys
    {
        auto cfg_path = tmp_dir / "case.cfg";
        std::ofstream out(cfg_path);
        out << "autostart=TRUE\n";
        out << "TrayMode=ON\n";
        out << "StartInTray=1\n";
        out << "Theme=LIGHT\n";
        out << "Mode=Client\n";
        out << "Audio_Mode=Fast\n";
        out << "Volume_Mode=FULL\n";
        out << "Mute_Local_Audio=ON\n";
        out << "Server_IP=10.0.0.1\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "case cfg loads successfully");
        auto& cfg = result.value();
        expect(!cfg.is_dark_theme(), "case cfg light theme is not dark");
        expect(cfg.autostart, "case cfg autostart TRUE is true");
        expect(cfg.traymode, "case cfg traymode ON is true");
        expect(cfg.startintray, "case cfg startintray 1 is true");
        expect(cfg.is_client(), "case cfg mode Client is client");
        expect(cfg.audio_mode == "fast", "case cfg audio_mode lowercased");
        expect(cfg.volume_mode == "full", "case cfg volume_mode lowercased");
        expect(cfg.mute_local_audio, "case cfg local audio mute is true");
        expect(cfg.server_ip == "10.0.0.1", "case cfg server_ip preserved");
        fs::remove(cfg_path);
    }

    // Test 6: Old SHARE_QUALITY key is ignored
    {
        auto cfg_path = tmp_dir / "old-quality-key.cfg";
        std::ofstream out(cfg_path);
        out << "AUTOSTART=true\n";
        out << "MODE=server\n";
        out << "SHARE_QUALITY=quality\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "old SHARE_QUALITY cfg loads successfully");
        expect(!result.value().has_audio_mode(), "old SHARE_QUALITY key is ignored");
        fs::remove(cfg_path);
    }

    // Test 7: False boolean values and invalid theme fall back safely
    {
        auto cfg_path = tmp_dir / "false-bools.cfg";
        std::ofstream out(cfg_path);
        out << "AUTOSTART=false\n";
        out << "TRAYMODE=no\n";
        out << "STARTINTRAY=off\n";
        out << "THEME=sepia\n";
        out << "MUTE_LOCAL_AUDIO=invalid\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "false bool cfg loads successfully");
        expect(!result.value().autostart, "false bool cfg autostart is false");
        expect(!result.value().traymode, "false bool cfg traymode is false");
        expect(!result.value().startintray, "false bool cfg startintray is false");
        expect(!result.value().is_dark_theme(), "invalid theme falls back to light");
        expect(!result.value().mute_local_audio, "invalid local audio mute falls back to false");
        fs::remove(cfg_path);
    }
    // Test 8: Unknown keys are silently ignored
    {
        auto cfg_path = tmp_dir / "unknown.cfg";
        std::ofstream out(cfg_path);
        out << "AUTOSTART=true\n";
        out << "UNKNOWN_KEY=some_value\n";
        out << "MODE=server\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "unknown keys cfg loads successfully");
        expect(result.value().autostart, "unknown keys cfg autostart works");
        expect(result.value().is_server(), "unknown keys cfg mode works");
        fs::remove(cfg_path);
    }

    // Test 9: GUI-only both mode parses correctly
    {
        auto cfg_path = tmp_dir / "both.cfg";
        std::ofstream out(cfg_path);
        out << "AUTOSTART=true\n";
        out << "MODE=both\n";
        out << "SERVER_IP=192.168.1.100\n";
        out.close();

        auto result = shareaudio::load_startup_config(cfg_path);
        expect(result.ok(), "both cfg loads successfully");
        expect(result.value().autostart, "both cfg autostart is true");
        expect(result.value().is_both(), "both cfg mode parses as both");
        expect(result.value().server_ip == "192.168.1.100", "both cfg server_ip matches");
        fs::remove(cfg_path);
    }

    fs::remove_all(tmp_dir);
}

} // namespace

int main()
{
    test_config();
    test_protocol();
    test_jitter_buffer();
    test_pcm_chunker();
    test_pcm_pipelines();
    test_system_volume_math();
    test_local_ip();
    test_recent_devices();
    test_app_controller();
    test_session_controller();
    test_session_controller_listen_loopback();
    test_audio_fakes();
    test_opus_codec();
    test_tcp_loopback();
    test_pcm_broadcast_server();
    test_hybrid_broadcast_server();
    test_session_controller_http_opus_fallback();
    test_console_commands();
    test_single_instance();
    test_startup_config();

    if (failures != 0) {
        std::cerr << failures << " test expectation(s) failed.\n";
        return 1;
    }
    std::cout << "All ShareAudioLite tests passed.\n";
    return 0;
}
