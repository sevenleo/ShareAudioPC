#include "app/SingleInstance.h"
#include "app/Config.h"
#include "app/Logger.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#endif

namespace shareaudio {
namespace {

constexpr auto stop_timeout = std::chrono::seconds(5);

#ifdef _WIN32
HANDLE instance_mutex = nullptr;
HANDLE stop_event = nullptr;
std::wstring ready_event_name;
bool background_child = false;
bool cleanup_registered = false;
#else
bool background_child = false;
#endif

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

#ifdef _WIN32
std::wstring stop_event_name(std::uint32_t pid)
{
    return L"Local\\ShareAudioLite.Stop." + std::to_wstring(pid);
}

bool is_our_process(std::uint32_t pid)
{
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process == nullptr) {
        return false;
    }
    wchar_t target[MAX_PATH] = {};
    DWORD target_size = MAX_PATH;
    wchar_t current[MAX_PATH] = {};
    const bool matches = QueryFullProcessImageNameW(process, 0, target, &target_size)
        && GetModuleFileNameW(nullptr, current, MAX_PATH) > 0
        && _wcsicmp(target, current) == 0;
    CloseHandle(process);
    return matches;
}
#endif

Result<void> terminate_process_by_pid(std::uint32_t pid)
{
#ifdef _WIN32
    if (!is_our_process(pid)) {
        return Result<void>::failure(make_error(
            ErrorCode::InvalidState, "PID file points to another executable; refusing to terminate it."));
    }
    HANDLE process = OpenProcess(SYNCHRONIZE | PROCESS_TERMINATE, FALSE, pid);
    if (process == nullptr) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to open the running ShareAudio process."));
    }
    if (HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, stop_event_name(pid).c_str())) {
        SetEvent(event);
        CloseHandle(event);
    }
    if (WaitForSingleObject(process, static_cast<DWORD>(stop_timeout.count() * 1000)) == WAIT_TIMEOUT) {
        Logger::warning("Running instance did not stop gracefully; terminating it.");
        if (!TerminateProcess(process, 1)) {
            CloseHandle(process);
            return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to terminate the running ShareAudio process."));
        }
        WaitForSingleObject(process, 1000);
    }
    CloseHandle(process);
#else
    kill(static_cast<pid_t>(pid), SIGTERM);
#endif
    return Result<void>::success();
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
#ifdef _WIN32
    if (stop_event != nullptr) {
        CloseHandle(stop_event);
        stop_event = nullptr;
    }
    if (instance_mutex != nullptr) {
        ReleaseMutex(instance_mutex);
        CloseHandle(instance_mutex);
        instance_mutex = nullptr;
    }
#endif
}

#ifdef _WIN32
class ControlLock {
public:
    ControlLock()
        : handle_(CreateMutexW(nullptr, FALSE, L"Local\\ShareAudioLite.InstanceControl"))
        , locked_(handle_ != nullptr && WaitForSingleObject(handle_, INFINITE) != WAIT_FAILED)
    {
    }
    ~ControlLock()
    {
        if (locked_) ReleaseMutex(handle_);
        if (handle_ != nullptr) CloseHandle(handle_);
    }
    [[nodiscard]] bool locked() const { return locked_; }

private:
    HANDLE handle_ {};
    bool locked_ {};
};
#endif

} // namespace

Result<void> enforce_single_instance()
{
#ifdef _WIN32
    if (instance_mutex != nullptr) {
        return Result<void>::success();
    }
    ControlLock lock;
    if (!lock.locked()) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to lock ShareAudio instance control."));
    }
#endif
    const auto path = default_pid_path();
    const auto current_pid = get_current_pid();

    try {
        if (std::filesystem::exists(path)) {
            std::ifstream file(path);
            std::uint32_t old_pid = 0;
            if (file >> old_pid) {
                file.close();
                if (old_pid != current_pid && is_process_running(old_pid)) {
                    Logger::info("Stopping existing instance with PID: " + std::to_string(old_pid));
                    auto stopped = terminate_process_by_pid(old_pid);
                    if (!stopped.ok()) {
                        return stopped;
                    }
                }
            }
        }

        std::filesystem::create_directories(path.parent_path());
#ifdef _WIN32
        instance_mutex = CreateMutexW(nullptr, FALSE, L"Local\\ShareAudioLite.Instance");
        const DWORD acquired = instance_mutex == nullptr ? WAIT_FAILED : WaitForSingleObject(instance_mutex, 1000);
        if (acquired != WAIT_OBJECT_0 && acquired != WAIT_ABANDONED) {
            return Result<void>::failure(make_error(ErrorCode::InvalidState, "Another ShareAudio instance is still running."));
        }
        stop_event = CreateEventW(nullptr, TRUE, FALSE, stop_event_name(current_pid).c_str());
        if (stop_event == nullptr) {
            cleanup_pid_file();
            return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to create the ShareAudio stop event."));
        }
#endif
        std::ofstream file(path, std::ios::trunc);
        if (!file) {
#ifdef _WIN32
            cleanup_pid_file();
#endif
            return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to write PID lockfile: " + path.string()));
        }
        file << current_pid << "\n";
        file.close();

#ifdef _WIN32
        if (!cleanup_registered) {
            std::atexit(cleanup_pid_file);
            cleanup_registered = true;
        }
#else
        std::atexit(cleanup_pid_file);
#endif

    } catch (const std::exception& e) {
        return Result<void>::failure(make_error(ErrorCode::IoError, std::string("Error enforcing single instance: ") + e.what()));
    }

    return Result<void>::success();
}

std::filesystem::path background_log_path()
{
    return default_config_path().parent_path() / "shareaudio.log";
}

Result<std::uint32_t> running_instance_pid()
{
    std::ifstream file(default_pid_path());
    std::uint32_t pid = 0;
    file >> pid;
    if (!is_process_running(pid)
#ifdef _WIN32
        || !is_our_process(pid)
#endif
    ) {
        std::error_code ignored;
        std::filesystem::remove(default_pid_path(), ignored);
        return Result<std::uint32_t>::success(0);
    }
    return Result<std::uint32_t>::success(pid);
}

Result<void> stop_running_instance()
{
#ifdef _WIN32
    ControlLock lock;
    if (!lock.locked()) {
        return Result<void>::failure(make_error(ErrorCode::IoError, "Unable to lock ShareAudio instance control."));
    }
#endif
    auto running = running_instance_pid();
    if (!running.ok() || running.value() == 0) {
        return running.ok() ? Result<void>::success() : Result<void>::failure(running.error());
    }
    auto stopped = terminate_process_by_pid(running.value());
    if (stopped.ok()) {
        std::error_code ignored;
        std::filesystem::remove(default_pid_path(), ignored);
    }
    return stopped;
}

void configure_background_child(std::string event_name)
{
    background_child = true;
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
#ifdef _WIN32
    ready_event_name.assign(event_name.begin(), event_name.end());
#else
    (void)event_name;
#endif
}

bool is_background_child()
{
    return background_child;
}

bool instance_stop_requested()
{
#ifdef _WIN32
    return stop_event != nullptr && WaitForSingleObject(stop_event, 0) == WAIT_OBJECT_0;
#else
    return false;
#endif
}

void wait_for_instance_stop_or_enter()
{
#ifdef _WIN32
    while (!instance_stop_requested()) {
        if (!background_child && _kbhit() && _getch() == '\r') {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
#else
    std::string line;
    std::getline(std::cin, line);
#endif
}

void notify_background_ready()
{
#ifdef _WIN32
    if (ready_event_name.empty()) return;
    if (HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, ready_event_name.c_str())) {
        SetEvent(event);
        CloseHandle(event);
    }
#endif
}

Result<std::uint32_t> launch_background_process()
{
#ifndef _WIN32
    return Result<std::uint32_t>::failure(make_error(
        ErrorCode::NotSupported, "--background is currently supported on Windows only."));
#else
    try {
        const auto log_path = background_log_path();
        std::filesystem::create_directories(log_path.parent_path());
        SECURITY_ATTRIBUTES attributes { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
        HANDLE log = CreateFileW(log_path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
            &attributes, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        HANDLE input = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
            &attributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (log == INVALID_HANDLE_VALUE || input == INVALID_HANDLE_VALUE) {
            if (log != INVALID_HANDLE_VALUE) CloseHandle(log);
            if (input != INVALID_HANDLE_VALUE) CloseHandle(input);
            return Result<std::uint32_t>::failure(make_error(ErrorCode::IoError, "Unable to open the background log."));
        }

        const std::wstring ready_name = L"Local\\ShareAudioLite.Ready."
            + std::to_wstring(get_current_pid()) + L"." + std::to_wstring(GetTickCount64());
        HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, ready_name.c_str());
        if (ready == nullptr) {
            CloseHandle(log);
            CloseHandle(input);
            return Result<std::uint32_t>::failure(make_error(ErrorCode::IoError, "Unable to create the background startup event."));
        }

        std::wstring command = GetCommandLineW();
        command += L" --background-child --background-ready-event " + ready_name;
        std::vector<wchar_t> mutable_command(command.begin(), command.end());
        mutable_command.push_back(L'\0');

        STARTUPINFOEXW startup {};
        startup.StartupInfo.cb = sizeof(startup);
        startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
        startup.StartupInfo.hStdInput = input;
        startup.StartupInfo.hStdOutput = log;
        startup.StartupInfo.hStdError = log;
        SIZE_T attribute_size = 0;
        InitializeProcThreadAttributeList(nullptr, 1, 0, &attribute_size);
        std::vector<std::byte> attribute_buffer(attribute_size);
        startup.lpAttributeList = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attribute_buffer.data());
        HANDLE inherited_handles[] { input, log };
        if (!InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attribute_size)
            || !UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                inherited_handles, sizeof(inherited_handles), nullptr, nullptr)) {
            CloseHandle(ready);
            CloseHandle(log);
            CloseHandle(input);
            return Result<std::uint32_t>::failure(make_error(ErrorCode::IoError, "Unable to configure background process handles."));
        }
        PROCESS_INFORMATION process {};
        BOOL created = CreateProcessW(nullptr, mutable_command.data(), nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW | CREATE_BREAKAWAY_FROM_JOB | EXTENDED_STARTUPINFO_PRESENT,
            nullptr, nullptr, &startup.StartupInfo, &process);
        if (!created) {
            mutable_command.assign(command.begin(), command.end());
            mutable_command.push_back(L'\0');
            created = CreateProcessW(nullptr, mutable_command.data(), nullptr, nullptr, TRUE,
                CREATE_NO_WINDOW | EXTENDED_STARTUPINFO_PRESENT,
                nullptr, nullptr, &startup.StartupInfo, &process);
        }
        DeleteProcThreadAttributeList(startup.lpAttributeList);
        CloseHandle(log);
        CloseHandle(input);
        if (!created) {
            CloseHandle(ready);
            return Result<std::uint32_t>::failure(make_error(ErrorCode::IoError, "Unable to start ShareAudio in the background."));
        }

        HANDLE waits[] { ready, process.hProcess };
        const DWORD wait = WaitForMultipleObjects(2, waits, FALSE, 15000);
        if (wait != WAIT_OBJECT_0) {
            TerminateProcess(process.hProcess, 2);
            WaitForSingleObject(process.hProcess, 1000);
            CloseHandle(ready);
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
            return Result<std::uint32_t>::failure(make_error(
                wait == WAIT_TIMEOUT ? ErrorCode::Timeout : ErrorCode::InvalidState,
                wait == WAIT_TIMEOUT ? "Background startup timed out. See " + log_path.string()
                                     : "Background session failed to start. See " + log_path.string()));
        }

        const auto pid = static_cast<std::uint32_t>(process.dwProcessId);
        CloseHandle(ready);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return Result<std::uint32_t>::success(pid);
    } catch (const std::exception& error) {
        return Result<std::uint32_t>::failure(make_error(
            ErrorCode::IoError, "Background startup failed: " + std::string(error.what())));
    }
#endif
}

} // namespace shareaudio
