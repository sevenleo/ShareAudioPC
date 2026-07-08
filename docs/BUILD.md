# Build

```powershell
cmake --preset windows-release
cmake --build --preset windows-release
```

```powershell
$env:APPDATA = Join-Path (Resolve-Path build/windows-release).Path "test-appdata"
New-Item -ItemType Directory -Force -Path $env:APPDATA | Out-Null
ctest --test-dir build/windows-release --output-on-failure
```

```powershell
cmake --install build/windows-release --config Release
```

```powershell
.\release\shareaudio_cli.exe help
```
