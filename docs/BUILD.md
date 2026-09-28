# Build And Portable Windows Package

This document describes the verified Windows build path. The repository presets use the bundled MinGW 13.1.0 and Qt 6.6.3 paths under `qt6`, enable CLI, GUI, tests, and libopus, and keep FetchContent updates disconnected.

## Prerequisites

- CMake `3.24` or newer available on `PATH`.
- The repository-local Qt/MinGW layout referenced by `CMakePresets.json`:
  - `qt6/Tools/mingw1310_64`;
  - `qt6/6.6.3/mingw_64`.
- Dependencies already available locally because the presets set `FETCHCONTENT_UPDATES_DISCONNECTED=ON`.
- No running `release\shareaudio_gui.exe` or `release\shareaudio_cli.exe` when the install step replaces executables.

## Recommended One-Command Build

From the repository root:

```bat
build.bat
```

`build.bat` performs these steps in order and stops on the first failure:

1. Configures the `windows-release` preset.
2. Builds all enabled targets.
3. Creates an isolated test `APPDATA` directory under `build/windows-release/test-appdata`.
4. Runs CTest with that isolated state so a normal user session does not interfere with single-instance tests.
5. Installs the portable package into the repository-root `release` directory.
6. Runs `release\shareaudio_cli.exe help` as a final executable smoke check.
7. Removes the obsolete `release-test` package so `release` is the only portable release directory.

## Manual PowerShell

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
cmake -E make_directory build/windows-release/test-appdata
cmake -E env APPDATA="$((Resolve-Path build/windows-release).Path)\test-appdata" ctest --test-dir build/windows-release --output-on-failure
cmake --install build/windows-release --config Release
.\release\shareaudio_cli.exe help
cmake -E rm -rf release-test
```

## Manual cmd.exe

```bat
cmake --preset windows-release
cmake --build --preset windows-release
cmake -E make_directory build\windows-release\test-appdata
cmake -E env "APPDATA=%CD%\build\windows-release\test-appdata" ctest --test-dir build/windows-release --output-on-failure
cmake --install build/windows-release --config Release
release\shareaudio_cli.exe help
cmake -E rm -rf release-test
```

## Debug Build

The debug preset installs to `release-debug` and otherwise enables the same major targets and Opus support:

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug
cmake -E make_directory build/windows-debug/test-appdata
cmake -E env APPDATA="$((Resolve-Path build/windows-debug).Path)\test-appdata" ctest --test-dir build/windows-debug --output-on-failure
cmake --install build/windows-debug --config Debug
```

## Produced Targets

| Target | Purpose |
| --- | --- |
| `shareaudio_core` | Shared Qt-free application, audio, protocol, network, storage, and session logic. |
| `shareaudio_cli` | Console transmitter/receiver and instance-control commands. |
| `shareaudio_gui` | Qt Widgets desktop application. |
| `shareaudio_tests` | Assert-style automated verification executable registered with CTest. |

## Portable `release` Contents

The install step is the authoritative packaging operation. It places `shareaudio_cli.exe`, `shareaudio_gui.exe`, and `shareaudio.cfg.example` in `release`, then runs `windeployqt` for the GUI. The directory also contains the required Qt DLLs, Qt plugins, and MinGW runtime DLLs. Copy the complete directory to another Windows computer; copying only the GUI executable is insufficient.

The install script excludes libopus development headers/libraries from the portable directory because Opus is linked into the application targets. Runtime configuration belongs in `release\shareaudio.cfg`, copied and edited from the example when needed.

## Test Isolation And Single-Instance Failures

The Windows single-instance implementation stores PID/control state under the application data environment and uses named mutexes/events. Running CTest against the normal user `APPDATA` while ShareAudioPC is open can make the three single-instance expectations fail even when the compiled code is unchanged. The commands above isolate `APPDATA` to make the test state deterministic. They do not terminate a running GUI automatically.

## Installation Failure Because The GUI Is Open

If CMake reports that `release\shareaudio_gui.exe` cannot be replaced, close ShareAudioPC normally, including any tray instance, and rerun only:

```powershell
cmake --install build/windows-release --config Release
```

Do not force-terminate the GUI during packaging because an active local endpoint mute or audio session should be allowed to run its normal cleanup path.
