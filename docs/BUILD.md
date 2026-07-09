# Build

## Windows

```bat
build.bat
```

## Manual PowerShell

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
cmake -E make_directory build/windows-release/test-appdata
cmake -E env APPDATA="$((Resolve-Path build/windows-release).Path)\test-appdata" ctest --test-dir build/windows-release --output-on-failure
cmake --install build/windows-release --config Release
.\release\shareaudio_cli.exe help
```

## Manual cmd.exe

```bat
cmake --preset windows-release
cmake --build --preset windows-release
cmake -E make_directory build\windows-release\test-appdata
cmake -E env "APPDATA=%CD%\build\windows-release\test-appdata" ctest --test-dir build\windows-release --output-on-failure
cmake --install build\windows-release --config Release
release\shareaudio_cli.exe help
```
