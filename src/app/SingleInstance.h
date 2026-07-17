#pragma once

#include "app/Result.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace shareaudio {

/**
 * Enforces that only a single instance of the application is running.
 * If a previous instance is detected, it terminates it before continuing.
 */
Result<void> enforce_single_instance();
Result<std::uint32_t> running_instance_pid();
Result<void> stop_running_instance();

void configure_background_child(std::string ready_event_name);
[[nodiscard]] bool is_background_child();
[[nodiscard]] bool instance_stop_requested();
void wait_for_instance_stop_or_enter();
void notify_background_ready();

Result<std::uint32_t> launch_background_process();
std::filesystem::path background_log_path();

} // namespace shareaudio
