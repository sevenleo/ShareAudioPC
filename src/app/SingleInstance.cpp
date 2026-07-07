#include "app/SingleInstance.h"
#include "app/Config.h"
#include "app/Logger.h"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif

namespace shareaudio {
namespace {

std::filesystem::path default_pid_path()
{
    return default_config_path().parent_path() / "shareaudio.pid";
}

std::uint32_t get_current_pid()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<std::uint32_t>(getpid());
#endif
}

bool is_process_running(std::uint32_t pid)
{
    if (pid == 0) return false;
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (process == nullptr) {
        return false;
    }
    DWORD exit_code = 0;
    BOOL result = GetExitCodeProcess(process, &exit_code);
    CloseHandle(process);
    return result && exit_code == STILL_ACTIVE;
#else
    return kill(static_cast<pid_t>(pid), 0) == 0;
#endif
}

void terminate_process_by_pid(std::uint32_t pid)
{
#ifdef _WIN32
    HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (process != nullptr) {
        TerminateProcess(process, 0);
        CloseHandle(process);
    }
#else
    kill(static_cast<pid_t>(pid), SIGKILL);
#endif
}

void cleanup_pid_file()
{
    try {
        const auto path = default_pid_path();
        if (std::filesystem::exists(path)) {
            // Only clean up if the PID inside matches ours (safety check)
            std::ifstream file(path);
            std::uint32_t pid = 0;
            if (file >> pid && pid == get_current_pid()) {
                file.close();
                std::filesystem::remove(path);
                Logger::info("PID lockfile cleaned up.");
            }
        }
    } catch (...) {
        // Suppress errors during global/exit cleanup
    }
}

} // namespace

Result<void> enforce_single_instance()
{
    const auto path = default_pid_path();
    const auto current_pid = get_current_pid();

    try {
        if (std::filesystem::exists(path)) {
            std::ifstream file(path);
            std::uint32_t old_pid = 0;
            if (file >> old_pid) {
                file.close();
                if (old_pid != current_pid && is_process_running(old_pid)) {
                    Logger::info("Detected existing instance running with PID: " + std::to_string(old_pid) + ". Terminating it...");
                    terminate_process_by_pid(old_pid);
                    
                    // Wait a bit for termination to finish
                    for (int i = 0; i < 10; ++i) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        if (!is_process_running(old_pid)) {
                            break;
                        }
                    }
                }
            }
        }

        // Write our PID
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::trunc);
        if (!file) {
            return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to write PID lockfile: " + path.string()));
        }
        file << current_pid << "\n";
        file.close();

        // Register exit cleanup hook
        std::atexit(cleanup_pid_file);

    } catch (const std::exception& e) {
        return Result<void>::failure(make_error(ErrorCode::IoError, std::string("Error enforcing single instance: ") + e.what()));
    }

    return Result<void>::success();
}

} // namespace shareaudio
